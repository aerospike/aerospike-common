/* 
 * Copyright 2008-2016 Aerospike, Inc.
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

#include <citrusleaf/alloc.h>

#include <aerospike/as_boolean.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_double.h>
#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_map.h>
#include <aerospike/as_string.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_map_hooks as_hashmap_map_hooks;

/******************************************************************************
 *	TYPES
 ******************************************************************************/

/******************************************************************************
 *	STATIC FUNCTIONS
 ******************************************************************************/

#define MIN_CAPACITY 1

static as_hashmap * as_hashmap_cons(as_hashmap * map, uint32_t capacity)
{
	map->count = 0;
	map->table_capacity = capacity > MIN_CAPACITY ? capacity : MIN_CAPACITY;

	size_t size = map->table_capacity * sizeof(as_hashmap_element);

	map->table = (as_hashmap_element *)cf_malloc(size);

	if (! map->table) {
		return NULL;
	}

	memset(map->table, 0, size);

	map->capacity_step = map->table_capacity / 2;

	if (map->capacity_step < 2) {
		map->capacity_step = 2;
	}

	map->extra_capacity = 0;
	map->extras = NULL;
	map->insert_at = 1; // can't be 0 since next = 0 means end of chain
	map->free_q = 0;

	return map;
}

static bool is_valid_key_type(const as_val * k)
{
	if (! k) {
		return false;
	}

	switch (as_val_type(k)) {
	case AS_NIL:
	case AS_BOOLEAN:
	case AS_INTEGER:
	case AS_DOUBLE:
	case AS_STRING:
	case AS_BYTES:
		return true;
	default:
		return false;
	}
}

// TODO - should probably be an as_val "class static" method that covers all
// types, but for now we'll locally cover only valid as_hashmap key types.
static bool eq_val(const as_val * v1, const as_val * v2)
{
	if (as_val_type(v1) != as_val_type(v2)) {
		return false;
	}

	switch (as_val_type(v1)) {
	case AS_NIL:
		return true;
	case AS_BOOLEAN:
		return as_boolean_get((const as_boolean *)v1) ==
				as_boolean_get((const as_boolean *)v2);
	case AS_INTEGER:
		return as_integer_get((const as_integer *)v1) ==
				as_integer_get((const as_integer *)v2);
	case AS_DOUBLE:
		return as_double_get((const as_double *)v1) ==
		as_double_get((const as_double *)v2);
	case AS_STRING:
		return 0 == strcmp(as_string_get((const as_string *)v1),
				as_string_get((const as_string *)v2));
	case AS_BYTES:
		return as_bytes_size((const as_bytes *)v1) ==
				as_bytes_size((const as_bytes *)v2) &&
				0 == memcmp(as_bytes_get((const as_bytes *)v1),
						as_bytes_get((const as_bytes *)v2),
						as_bytes_size((const as_bytes *)v1));
	default:
		// Should never get here.
		return false;
	}
}

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

as_hashmap * as_hashmap_init(as_hashmap * map, uint32_t capacity)
{
	if (! map) {
		return NULL;
	}

	as_map_cons((as_map *)map, false, NULL, &as_hashmap_map_hooks);

	return as_hashmap_cons(map, capacity);
}

as_hashmap * as_hashmap_new(uint32_t capacity)
{
	as_hashmap * map = (as_hashmap *)cf_malloc(sizeof(as_hashmap));

	if (! map) {
		return NULL;
	}

	as_map_cons((as_map *)map, true, NULL, &as_hashmap_map_hooks);

	return as_hashmap_cons(map, capacity);
}

bool as_hashmap_release(as_hashmap * map)
{
	if (! map) {
		return false;
	}

	as_hashmap_clear(map);
	cf_free(map->table);

	return true;
}

void as_hashmap_destroy(as_hashmap * map) {
	as_map_destroy((as_map *) map);
}

/******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

/**
 *	@todo Fill In
 */
uint32_t as_hashmap_hashcode(const as_hashmap * map)
{
	return 1;
}

uint32_t as_hashmap_size(const as_hashmap * map)
{
	return map ? map->count : 0;
}

/*******************************************************************************
 *	ACCESSOR & MODIFICATION FUNCTIONS
 ******************************************************************************/

int as_hashmap_set(as_hashmap * map, const as_val * k, const as_val * v)
{
	if (! map) {
		return -1;
	}

	if (! is_valid_key_type(k)) {
		return -1;
	}

	uint32_t h = as_val_hashcode(k);
	uint32_t i = h % map->table_capacity;

	as_hashmap_element * e = &map->table[i];

	// If the slot in the main table is empty - use it.
	if (! e->p_key) {
		map->count++;

		e->p_key = (as_val *)k;
		e->p_val = (as_val *)v;

		return 0;
	}

	// There are existing elements hashed to this slot - follow the chain.
	while (true) {
		// If we find our key, replace the existing key and value.
		if (eq_val(e->p_key, k)) {
			as_val_destroy(e->p_key);
			as_val_destroy(e->p_val);
			e->p_key = (as_val *)k;
			e->p_val = (as_val *)v;

			return 0;
		}

		if (e->next == 0) {
			break;
		}

		e = &map->extras[e->next];
	}

	// We did not find our key among the existing elements - add it to the end
	// of the chain.

	as_hashmap_element * prev_e = e;

	// If there's space on the free-q, use it.
	if (map->free_q != 0) {
		map->count++;
		prev_e->next = map->free_q;

		e = &map->extras[map->free_q];

		map->free_q = e->next;

		e->p_key = (as_val *)k;
		e->p_val = (as_val *)v;
		e->next = 0;

		return 0;
	}

	// No space on the free-q, must insert at end.

	uint32_t cur_end = prev_e->next;

	// prev_e could be a pointer into extras, which might be realloc'd, so we
	// modify prev_e->next first (but revert if malloc/realloc fails).
	prev_e->next = map->insert_at;

	// First grow the extra capacity if necessary.
	// TODO - vertical scaling.
	if (map->insert_at >= map->extra_capacity) {
		size_t orig_size = map->extra_capacity * sizeof(as_hashmap_element);
		uint32_t extra_capacity = map->extra_capacity + map->capacity_step;
		size_t size = extra_capacity * sizeof(as_hashmap_element);

		if (map->extras) {
			as_hashmap_element * extras =
					(as_hashmap_element *)cf_realloc(map->extras, size);

			if (! extras) {
				prev_e->next = cur_end;
				return -1;
			}

			map->extras = extras;
			memset((uint8_t*)map->extras + orig_size, 0, size - orig_size);
		}
		else {
			if (! (map->extras = (as_hashmap_element *)cf_malloc(size))) {
				prev_e->next = cur_end;
				return -1;
			}

			memset(map->extras, 0, size);
		}

		map->extra_capacity = extra_capacity;
	}

	map->count++;

	e = &map->extras[map->insert_at++];

	e->p_key = (as_val *)k;
	e->p_val = (as_val *)v;

	return 0;
}

as_val * as_hashmap_get(const as_hashmap * map, const as_val * k)
{
	if (! map) {
		return NULL;
	}

	if (! is_valid_key_type(k)) {
		return NULL;
	}

	uint32_t h = as_val_hashcode(k);
	uint32_t i = h % map->table_capacity;

	as_hashmap_element * e = &map->table[i];

	// If the table slot is empty, the key isn't in the map.
	if (! e->p_key) {
		return NULL;
	}

	// Follow the chain.
	while (true) {
		if (eq_val(e->p_key, k)) {
			return e->p_val;
		}

		if (e->next == 0) {
			break;
		}

		e = &map->extras[e->next];
	}

	return NULL;
}

int as_hashmap_clear(as_hashmap * map)
{
	if (! map ) {
		return -1;
	}

	for (uint32_t i = 0; i < map->table_capacity; i++) {
		as_hashmap_element * e = &map->table[i];

		if (e->p_key) {
			as_val_destroy(e->p_key);
			as_val_destroy(e->p_val);
			e->p_key = NULL;
			e->p_val = NULL;
			e->next = 0;
		}
	}

	for (uint32_t i = 1; i < map->insert_at; i++) {
		as_hashmap_element * e = &map->extras[i];

		if (e->p_key) {
			as_val_destroy(e->p_key);
			as_val_destroy(e->p_val);
		}
	}

	map->count = 0;

	if (map->extras) {
		cf_free(map->extras);
		map->extras = NULL;
	}

	map->extra_capacity = 0;
	map->insert_at = 1;
	map->free_q = 0;

	return 0;
}

int as_hashmap_remove(as_hashmap * map, const as_val * k)
{
	if (! map) {
		return -1;
	}

	if (! is_valid_key_type(k)) {
		return -1; // or 0?
	}

	uint32_t h = as_val_hashcode(k);
	uint32_t i = h % map->table_capacity;

	as_hashmap_element * e = &map->table[i];

	// If the table slot is empty, the key isn't in the map.
	if (! e->p_key) {
		return 0;
	}

	// If the table slot has our key, remove it and repair the chain.
	if (eq_val(e->p_key, k)) {
		map->count--;

		as_val_destroy(e->p_key);
		as_val_destroy(e->p_val);

		// There was no chain - we're done.
		if (e->next == 0) {
			e->p_key = NULL;
			e->p_val = NULL;

			return 0;
		}

		// Copy the first element in the chain into the main table slot and free
		// the slot it occupied.

		uint32_t free_i = e->next;
		as_hashmap_element * free_e = &map->extras[free_i];

		*e = *free_e;

		free_e->p_key = NULL;
		free_e->p_val = NULL;
		free_e->next = map->free_q;
		map->free_q = free_i;

		return 0;
	}

	// The table slot didn't have our key, follow the chain.
	while (true) {
		if (e->next == 0) {
			return 0;
		}

		as_hashmap_element * prev_e = e;
		uint32_t i = e->next;

		e = &map->extras[i];

		// If we find our key, free its slot and repair the chain by changing
		// the previous item's next "pointer".

		if (eq_val(e->p_key, k)) {
			map->count--;

			as_val_destroy(e->p_key);
			as_val_destroy(e->p_val);
			e->p_key = NULL;
			e->p_val = NULL;

			prev_e->next = e->next;

			e->next = map->free_q;
			map->free_q = i;

			return 0;
		}
	}

	return 0;
}

/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

bool as_hashmap_foreach(const as_hashmap * map, as_map_foreach_callback callback, void * udata)
{
	if (! map) {
		return false;
	}

	for (uint32_t i = 0; i < map->table_capacity; i++) {
		as_hashmap_element * e = &map->table[i];

		if (! e->p_key) {
			continue;
		}

		if (! callback((const as_val *)e->p_key, (const as_val *)e->p_val, udata)) {
			return false;
		}
	}

	for (uint32_t i = 1; i < map->insert_at; i++) {
		as_hashmap_element * e = &map->extras[i];

		if (! e->p_key) {
			continue;
		}

		if (! callback((const as_val *)e->p_key, (const as_val *)e->p_val, udata)) {
			return false;
		}
	}

	return true;
}
