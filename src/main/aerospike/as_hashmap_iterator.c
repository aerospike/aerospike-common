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

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_pair.h>

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_iterator_hooks as_hashmap_iterator_hooks;

/******************************************************************************
 *	STATIC FUNCTIONS
 *****************************************************************************/

static void as_hashmap_iterator_reset(as_hashmap_iterator * iterator)
{
	iterator->map = NULL;
	iterator->curr = NULL;
	iterator->count = 0;
	iterator->table_pos = 0;
	iterator->extras_pos = 1;
}

static bool as_hashmap_iterator_seek(as_hashmap_iterator * iterator)
{
	// If curr is set, that means we have a value ready to be read.
	if (iterator->curr) {
		return true;
	}

	const as_hashmap * map = iterator->map;

	// We have already found all the entries.
	if (iterator->count >= map->count) {
		return false;
	}

	while (iterator->table_pos < map->table_capacity) {
		as_hashmap_element * e = &map->table[iterator->table_pos++];

		if (e->p_key) {
			iterator->curr = e;
			iterator->count++;
			return true;
		}
	}

	while (iterator->extras_pos < map->insert_at) {
		as_hashmap_element * e = &map->extras[iterator->extras_pos++];

		if (e->p_key) {
			iterator->curr = e;
			iterator->count++;
			return true;
		}
	}

	return false;
}

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

as_hashmap_iterator * as_hashmap_iterator_init(as_hashmap_iterator * iterator, const as_hashmap * map)
{
	if (! iterator) {
		return NULL;
	}

	as_iterator_init((as_iterator *)iterator, false, NULL, &as_hashmap_iterator_hooks);
	as_hashmap_iterator_reset(iterator);
	iterator->map = map;

	return iterator;
}

as_hashmap_iterator * as_hashmap_iterator_new(const as_hashmap * map)
{
	as_hashmap_iterator * iterator = (as_hashmap_iterator *)cf_malloc(sizeof(as_hashmap_iterator));

	if (! iterator) {
		return NULL;
	}

	as_iterator_init((as_iterator *)iterator, true, NULL, &as_hashmap_iterator_hooks);
	as_hashmap_iterator_reset(iterator);
	iterator->map = map;

	return iterator;
}

bool as_hashmap_iterator_release(as_hashmap_iterator * iterator)
{
	as_hashmap_iterator_reset(iterator);

	return true;
}

void as_hashmap_iterator_destroy(as_hashmap_iterator * iterator)
{
	as_iterator_destroy((as_iterator *)iterator);
}

bool as_hashmap_iterator_has_next(const as_hashmap_iterator * iterator)
{
	return as_hashmap_iterator_seek((as_hashmap_iterator *)iterator);
}

const as_val * as_hashmap_iterator_next(as_hashmap_iterator * iterator)
{
	if (! as_hashmap_iterator_seek(iterator)) {
		return NULL;
	}

	as_hashmap_element * e = iterator->curr;

	iterator->curr = NULL; // consume the value, so we can get the next one

	as_pair_init(&iterator->pair, e->p_key, e->p_val);

	return (const as_val *)&iterator->pair;
}
