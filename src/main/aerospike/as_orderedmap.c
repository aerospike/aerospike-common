/*
 * Copyright 2022 Aerospike, Inc.
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


/******************************************************************************
 *	STATIC FUNCTIONS
 ******************************************************************************/

static as_orderedmap*
as_orderedmap_cons(as_orderedmap* map, uint32_t capacity)
{
	map->count = 0;
	map->capacity = ((capacity + 8) / 8) * 8; // can add 1 without realloc

	size_t size = map->capacity * sizeof(as_val*) * 2;

	map->table = (as_val**)cf_malloc(size);

	if (map->table == NULL) {
		return NULL;
	}

	return map;
}

static bool
is_valid_key_type(const as_val* k)
{
	if (k == NULL) {
		return false;
	}

	switch (as_val_type(k)) {
	case AS_NIL:
	case AS_BOOLEAN:
	case AS_INTEGER:
	case AS_LIST:
	case AS_DOUBLE:
	case AS_STRING:
	case AS_BYTES:
	case AS_GEOJSON:
		break;
	default:
		return false;
	}

	return true;
}

static bool
val_find(uint32_t count, const as_val* v, as_val** table, uint32_t* idx_r,
		bool check_last_first)
{
	if (count == 0) {
		*idx_r = 0;
		return false;
	}

	if (check_last_first) {
		msgpack_compare_t cmp = as_val_cmp(v, table[(count - 1) * 2]);

		switch (cmp) {
		case MSGPACK_COMPARE_EQUAL:
			*idx_r = count - 1;
			return true;
		case MSGPACK_COMPARE_GREATER:
			*idx_r = count;
			return false;
		case MSGPACK_COMPARE_LESS:
			count--;

			if (count == 0) {
				*idx_r = 0;
				return false;
			}

			break;
		default:
			*idx_r = UINT32_MAX;
			return false;
		}
	}

	uint32_t lower = 0;
	uint32_t idx = count / 2;
	uint32_t upper = count;

	while (true) {
		msgpack_compare_t cmp = as_val_cmp(v, table[idx * 2]);

		if (cmp == MSGPACK_COMPARE_EQUAL) {
			*idx_r = idx;
			return true;
		}

		if (cmp == MSGPACK_COMPARE_GREATER) {
			if (idx >= upper - 1) {
				*idx_r = idx + 1;
				return false;
			}

			lower = idx;
			idx += upper;
			idx /= 2;
		}
		else if (cmp == MSGPACK_COMPARE_LESS) {
			if (idx == lower) {
				*idx_r = idx;
				return false;
			}

			upper = idx;
			idx += lower;
			idx /= 2;
		}
		else {
			break;
		}
	}

	*idx_r = UINT32_MAX; // error

	return false;
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
	return map == NULL ? 0 : map->count;
}

as_val*
as_orderedmap_get(const as_orderedmap* map, const as_val* key)
{
	if (map == NULL || ! is_valid_key_type(key)) {
		return NULL;
	}

	uint32_t idx;
	bool found = val_find(map->count, key, map->table, &idx, false);

	if (! found) {
		return NULL;
	}

	return (as_val*)map->table[idx * 2 + 1];
}

int
as_orderedmap_set(as_orderedmap* map, const as_val* key, const as_val* val)
{
	if (map == NULL || ! is_valid_key_type(key)) {
		return -1;
	}

	as_val* cval = (as_val*)(val != NULL ? val : &as_nil);
	as_val* ckey = (as_val*)key;
	uint32_t idx;
	bool found = val_find(map->count, key, map->table, &idx, true);

	if (idx == UINT32_MAX) {
		return -1;
	}

	idx *= 2;

	if (found) {
		as_val_destroy(map->table[idx]);
		as_val_destroy(map->table[idx + 1]);
		map->table[idx] = ckey;
		map->table[idx + 1] = cval;

		return 0;
	}

	if (map->count == map->capacity) {
		map->capacity *= 2;

		as_val** table = (as_val**)cf_realloc(map->table,
				map->capacity * sizeof(as_val*) * 2);

		if (table == NULL) {
			return -1;
		}

		map->table = table;
	}

	memmove(&map->table[idx + 2], &map->table[idx],
			sizeof(as_val*) * (map->count * 2 - idx));
	map->table[idx] = ckey;
	map->table[idx + 1] = cval;
	map->count++;

	return 0;
}

int
as_orderedmap_clear(as_orderedmap* map)
{
	if (map == NULL) {
		return -1;
	}

	for (uint32_t i = 0; i < map->count * 2; i++) {
		as_val_destroy(map->table[i]);
	}

	map->count = 0;

	return 0;
}

int
as_orderedmap_remove(as_orderedmap* map, const as_val* key)
{
	if (map == NULL || ! is_valid_key_type(key)) {
		return -1;
	}

	uint32_t idx;
	bool found = val_find(map->count, key, map->table, &idx, false);

	if (! found) {
		return 0;
	}

	idx *= 2;
	as_val_destroy(map->table[idx]);
	as_val_destroy(map->table[idx + 1]);
	memmove(&map->table[idx], &map->table[idx + 2],
			sizeof(as_val*) * (map->count * 2 - (idx + 2)));
	map->count--;

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

	for (uint32_t i = 0; i < map->count; i++) {
		if (! callback(map->table[i * 2], map->table[i * 2 + 1], udata)) {
			return false;
		}
	}

	return true;
}

as_orderedmap_iterator*
as_orderedmap_iterator_init(as_orderedmap_iterator* it,
		const as_orderedmap* map)
{
	if (it == NULL) {
		return NULL;
	}

	as_iterator_init((as_iterator*)it, false, NULL,
			&as_orderedmap_iterator_hooks);
	it->idx = 0;
	it->map = map;

	return it;
}

as_orderedmap_iterator*
as_orderedmap_iterator_new(const as_orderedmap* map)
{
	as_orderedmap_iterator* it =
			(as_orderedmap_iterator*)cf_malloc(sizeof(as_orderedmap_iterator));

	if (it == NULL) {
		return NULL;
	}

	as_iterator_init((as_iterator*)it, true, NULL,
			&as_orderedmap_iterator_hooks);
	it->idx = 0;
	it->map = map;

	return it;
}

bool
as_orderedmap_iterator_release(as_orderedmap_iterator* it)
{
	it->map = NULL;
	it->idx = 0;
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
	return it->idx < it->map->count;
}

const as_val*
as_orderedmap_iterator_next(as_orderedmap_iterator* it)
{
	if (it->idx >= it->map->count) {
		return NULL;
	}

	as_pair_init(&it->pair, it->map->table[it->idx * 2],
			it->map->table[it->idx * 2 + 1]);
	it->idx++;

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
