/*
 * Copyright 2022-2024 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <aerospike/as_orderedmap.h>

#include <aerospike/as_std.h>
#include <stddef.h>
#include <string.h>

#include <aerospike/as_boolean.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_double.h>
#include <aerospike/as_geojson.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>
#include <aerospike/as_map_iterator.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_nil.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_string.h>
#include <aerospike/as_val.h>
#include <citrusleaf/alloc.h>


/******************************************************************************
 *	TYPES
 ******************************************************************************/

static const as_map_hooks as_orderedmap_map_hooks;
static const as_iterator_hooks as_orderedmap_iterator_hooks;

#define HOLD_TABLE_CAP 1000

/******************************************************************************
 *	STATIC FUNCTIONS
 ******************************************************************************/

static as_orderedmap*
as_orderedmap_cons(as_orderedmap* map, uint32_t capacity)
{
	map->count = 0;
	map->capacity = ((capacity + 8) / 8) * 8; // can add 1 without realloc

	size_t size = map->capacity * sizeof(map_entry);

	map->table = (map_entry*)cf_malloc(size);

	if (map->table == NULL) {
		return NULL;
	}

	map->hold_count = 0;
	map->hold_table = NULL;
	map->hold_locations = NULL;

	return map;
}

static bool
is_valid_key_type(const as_val* key)
{
	if (key == NULL) {
		return false;
	}

	switch (as_val_type(key)) {
	case AS_INTEGER:
	case AS_STRING:
		break;
	case AS_BYTES:
		return as_bytes_get_type((as_bytes*)key) == AS_BYTES_BLOB;
	default:
		return false;
	}

	return true;
}

static bool
key_find(const map_entry* table, uint32_t count, const as_val* key,
		uint32_t* ix_r, bool check_last_first)
{
	int64_t low = 0;
	int64_t high = (int64_t)count - 1;

	while (low <= high) {
		int64_t ix;

		// On the first iteration, probe at the end when 'check_last_first' is
		// set. Otherwise, fall back to binary search.
		if (check_last_first) {
			check_last_first = false;
			ix = high;
		}
		else {
			ix = (low + high) / 2;
		}

		msgpack_compare_t cmp = as_val_cmp(key, table[ix].key);

		if (cmp == MSGPACK_COMPARE_GREATER) {
			low = ix + 1;
		}
		else if (cmp == MSGPACK_COMPARE_LESS) {
			high = ix - 1;
		}
		else if (cmp == MSGPACK_COMPARE_EQUAL) {
			*ix_r = (uint32_t)ix;
			return true;
		}
		else {
			*ix_r = UINT32_MAX; // error
			return false;
		}
	}

	*ix_r = (uint32_t)low; // location to insert the key at

	return false;
}

static bool
as_orderedmap_merge(as_orderedmap* map)
{
	if (map->hold_count == 0) {
		return true;
	}

	uint32_t new_capacity = map->count + map->hold_count;

	if (new_capacity < map->capacity) {
		new_capacity = map->capacity;
	}

	map_entry* new_table = cf_malloc(new_capacity * sizeof(map_entry));

	if (new_table == NULL) {
		return false;
	}

	uint32_t src_ix = 0;
	uint32_t dst_ix = 0;

	for (uint32_t ix = 0; ix < map->hold_count; ix++) {
		uint32_t n_entries = map->hold_locations[ix] - src_ix;

		memcpy(new_table + dst_ix, map->table + src_ix,
				n_entries * sizeof(map_entry));

		src_ix += n_entries;
		dst_ix += n_entries;

		new_table[dst_ix].key = map->hold_table[ix].key;
		new_table[dst_ix].value = map->hold_table[ix].value;

		dst_ix++;
	}

	memcpy(new_table + dst_ix, map->table + src_ix,
			(map->count - src_ix) * sizeof(map_entry));

	cf_free(map->table);

	map->count += map->hold_count;
	map->capacity = new_capacity;
	map->table = new_table;

	map->hold_count = 0;

	return true;
}


/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

as_orderedmap*
as_orderedmap_init(as_orderedmap* map, uint32_t capacity)
{
	if (map == NULL) {
		return NULL;
	}

	as_map_cons((as_map*)map, false, 1, &as_orderedmap_map_hooks);

	return as_orderedmap_cons(map, capacity);
}

as_orderedmap*
as_orderedmap_new(uint32_t capacity)
{
	as_orderedmap* map = (as_orderedmap*)cf_malloc(sizeof(as_orderedmap));

	if (map == NULL) {
		return NULL;
	}

	as_map_cons((as_map*)map, true, 1, &as_orderedmap_map_hooks);

	return as_orderedmap_cons(map, capacity);
}

bool
as_orderedmap_release(as_orderedmap* map)
{
	if (map == NULL) {
		return false;
	}

	as_orderedmap_clear(map);
	cf_free(map->table);

	if (map->hold_table != NULL) {
		cf_free(map->hold_table);
		cf_free(map->hold_locations);
	}

	return true;
}

void
as_orderedmap_destroy(as_orderedmap* map)
{
	as_map_destroy((as_map*)map);
}

uint32_t
as_orderedmap_size(const as_orderedmap* map)
{
	return map == NULL ? 0 : map->count + map->hold_count;
}

as_val*
as_orderedmap_get(const as_orderedmap* map, const as_val* key)
{
	if (map == NULL || ! is_valid_key_type(key)) {
		return NULL;
	}

	uint32_t ix;

	if (key_find(map->table, map->count, key, &ix, false)) {
		return (as_val*)map->table[ix].value;
	}

	if (key_find(map->hold_table, map->hold_count, key, &ix, false)) {
		return (as_val*)map->hold_table[ix].value;
	}

	return NULL;
}

int
as_orderedmap_set(as_orderedmap* map, const as_val* key, const as_val* val)
{
	if (map == NULL || ! is_valid_key_type(key)) {
		return -1;
	}

	as_val* cval = (as_val*)(val != NULL ? val : &as_nil);
	as_val* ckey = (as_val*)key;
	uint32_t ix;
	bool found = key_find(map->table, map->count, key, &ix, true);

	if (ix == UINT32_MAX) {
		return -1;
	}

	if (found) {
		as_val_destroy(map->table[ix].key);
		as_val_destroy(map->table[ix].value);
		map->table[ix].key = ckey;
		map->table[ix].value = cval;

		return 0;
	}

	if (ix + HOLD_TABLE_CAP > map->count) {
		// Near end of main table - insert directly.

		if (map->count == map->capacity) {
			map->capacity *= 2;

			map_entry* table = (map_entry*)cf_realloc(map->table,
					map->capacity * sizeof(map_entry));

			if (table == NULL) {
				return -1;
			}

			map->table = table;
		}

		memmove(&map->table[ix + 1], &map->table[ix],
				sizeof(map_entry) * (map->count - ix));
		map->table[ix].key = ckey;
		map->table[ix].value = cval;
		map->count++;

		return 0;
	}

	// Far from end of main table - insert in hold table.

	if (map->hold_table == NULL) {
		map->hold_table = cf_malloc(HOLD_TABLE_CAP * sizeof(map_entry));
		map->hold_locations = cf_malloc(HOLD_TABLE_CAP * sizeof(uint32_t));
	}

	uint32_t hold_ix;

	found = key_find(map->hold_table, map->hold_count, key, &hold_ix, false);

	if (hold_ix == UINT32_MAX) {
		return -1;
	}

	if (found) {
		as_val_destroy(map->hold_table[hold_ix].key);
		as_val_destroy(map->hold_table[hold_ix].value);
		map->hold_table[hold_ix].key = ckey;
		map->hold_table[hold_ix].value = cval;

		return 0;
	}

	if (map->hold_count == HOLD_TABLE_CAP) {
		return -1; // previous merge failed
	}

	memmove(&map->hold_table[hold_ix + 1], &map->hold_table[hold_ix],
			sizeof(map_entry) * (map->hold_count - hold_ix));
	map->hold_table[hold_ix].key = ckey;
	map->hold_table[hold_ix].value = cval;

	memmove(&map->hold_locations[hold_ix + 1], &map->hold_locations[hold_ix],
			sizeof(uint32_t) * (map->hold_count - hold_ix));
	map->hold_locations[hold_ix] = ix;

	map->hold_count++;

	if (map->hold_count == HOLD_TABLE_CAP) {
		as_orderedmap_merge(map);
		// Ignore merge allocation failure, next insert will fail.
	}

	return 0;
}

int
as_orderedmap_clear(as_orderedmap* map)
{
	if (map == NULL) {
		return -1;
	}

	for (uint32_t ix = 0; ix < map->count; ix++) {
		as_val_destroy(map->table[ix].key);
		as_val_destroy(map->table[ix].value);
	}

	map->count = 0;

	for (uint32_t ix = 0; ix < map->hold_count; ix++) {
		as_val_destroy(map->hold_table[ix].key);
		as_val_destroy(map->hold_table[ix].value);
	}

	map->hold_count = 0;

	return 0;
}

int
as_orderedmap_remove(as_orderedmap* map, const as_val* key)
{
	if (map == NULL || ! is_valid_key_type(key)) {
		return -1;
	}

	if (! as_orderedmap_merge(map)) {
		return -1;
	}

	uint32_t ix;

	if (key_find(map->table, map->count, key, &ix, false)) {
		as_val_destroy(map->table[ix].key);
		as_val_destroy(map->table[ix].value);
		memmove(&map->table[ix], &map->table[ix + 1],
				sizeof(map_entry) * (map->count - (ix + 1)));
		map->count--;
	}

	return 0;
}


/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

bool
as_orderedmap_foreach(const as_orderedmap* map,
		as_map_foreach_callback callback, void* udata)
{
	if (map == NULL) {
		return false;
	}

	if (! as_orderedmap_merge((as_orderedmap*)map)) {
		return false;
	}

	for (uint32_t ix = 0; ix < map->count; ix++) {
		if (! callback(map->table[ix].key, map->table[ix].value, udata)) {
			return false;
		}
	}

	return true;
}

as_orderedmap_iterator*
as_orderedmap_iterator_init(as_orderedmap_iterator* it,
		const as_orderedmap* map)
{
	if (map != NULL && ! as_orderedmap_merge((as_orderedmap*)map)) {
		return NULL;
	}

	if (it == NULL) {
		return NULL;
	}

	as_iterator_init((as_iterator*)it, false, NULL,
			&as_orderedmap_iterator_hooks);
	it->ix = 0;
	it->map = map;

	return it;
}

as_orderedmap_iterator*
as_orderedmap_iterator_new(const as_orderedmap* map)
{
	if (map != NULL && ! as_orderedmap_merge((as_orderedmap*)map)) {
		return NULL;
	}

	as_orderedmap_iterator* it =
			(as_orderedmap_iterator*)cf_malloc(sizeof(as_orderedmap_iterator));

	if (it == NULL) {
		return NULL;
	}

	as_iterator_init((as_iterator*)it, true, NULL,
			&as_orderedmap_iterator_hooks);
	it->ix = 0;
	it->map = map;

	return it;
}

bool
as_orderedmap_iterator_release(as_orderedmap_iterator* it)
{
	it->map = NULL;
	it->ix = 0;
	return true;
}

void
as_orderedmap_iterator_destroy(as_orderedmap_iterator* it)
{
	as_iterator_destroy((as_iterator*)it);
}

bool
as_orderedmap_iterator_has_next(const as_orderedmap_iterator* it)
{
	return it->ix < it->map->count;
}

const as_val*
as_orderedmap_iterator_next(as_orderedmap_iterator* it)
{
	if (it->ix >= it->map->count) {
		return NULL;
	}

	as_pair_init(&it->pair, it->map->table[it->ix].key,
			it->map->table[it->ix].value);
	it->ix++;

	return (as_val*)&it->pair;
}


/*******************************************************************************
 *	HOOKS
 ******************************************************************************/

static bool
_map_destroy(as_map* map)
{
	return as_orderedmap_release((as_orderedmap*)map);
}

static uint32_t
_map_hashcode(const as_map* map)
{
	return 1;
}

static uint32_t
_map_size(const as_map* map)
{
	return as_orderedmap_size((const as_orderedmap*)map);
}

static int
_map_set(as_map* map, const as_val* key, const as_val* val)
{
	return as_orderedmap_set((as_orderedmap*)map, key, val);
}

static as_val*
_map_get(const as_map* map, const as_val* key)
{
	return as_orderedmap_get((as_orderedmap*)map, key);
}

static int
_map_clear(as_map* map)
{
	return as_orderedmap_clear((as_orderedmap*)map);
}

static int
_map_remove(as_map *map, const as_val* key)
{
	return as_orderedmap_remove((as_orderedmap*)map, key);
}

static void
_map_set_flags(as_map *map, uint32_t flags)
{
	as_orderedmap_set_flags((as_orderedmap*)map, flags);
}

static bool
_map_foreach(const as_map* map, as_map_foreach_callback callback, void* udata)
{
	return as_orderedmap_foreach((const as_orderedmap*)map, callback, udata);
}

static as_map_iterator*
_map_iterator_new(const as_map* map)
{
	return (as_map_iterator*)as_orderedmap_iterator_new(
			(const as_orderedmap*)map);
}

static as_map_iterator*
_map_iterator_init(const as_map* map, as_map_iterator* it)
{
	return (as_map_iterator*)as_orderedmap_iterator_init(
			(as_orderedmap_iterator*)it, (as_orderedmap*)map);
}

static const as_map_hooks as_orderedmap_map_hooks = {

	/***************************************************************************
	 *	instance hooks
	 **************************************************************************/

	.destroy	= _map_destroy,

	/***************************************************************************
	 *	info hooks
	 **************************************************************************/

	.hashcode	= _map_hashcode,
	.size		= _map_size,

	/***************************************************************************
	 *	accessor and modifier hooks
	 **************************************************************************/

	.set		= _map_set,
	.get		= _map_get,
	.clear		= _map_clear,
	.remove		= _map_remove,
	.set_flags	= _map_set_flags,

	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/

	.foreach		= _map_foreach,
	.iterator_new	= _map_iterator_new,
	.iterator_init	= _map_iterator_init,
};

static bool
_iterator_destroy(as_iterator* it)
{
	return as_orderedmap_iterator_release((as_orderedmap_iterator*)it);
}

static bool
_iterator_has_next(const as_iterator* it)
{
	return as_orderedmap_iterator_has_next((const as_orderedmap_iterator *)it);
}

static const as_val*
_iterator_next(as_iterator* it)
{
	return as_orderedmap_iterator_next((as_orderedmap_iterator*) it);
}

static const as_iterator_hooks as_orderedmap_iterator_hooks = {
	.destroy    = _iterator_destroy,
	.has_next   = _iterator_has_next,
	.next       = _iterator_next
};
