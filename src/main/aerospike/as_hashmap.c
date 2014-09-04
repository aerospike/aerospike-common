/* 
 * Copyright 2008-2014 Aerospike, Inc.
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
#include <citrusleaf/cf_shash.h>

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdlib.h>

#include "internal.h"

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_map_hooks as_hashmap_map_hooks;

/******************************************************************************
 *	TYPES
 ******************************************************************************/

typedef struct {
	void *                  udata;
	as_map_foreach_callback callback;
} as_hashmap_shash_foreach_context;

/******************************************************************************
 *	STATIC FUNCTIONS
 ******************************************************************************/

static uint32_t as_hashmap_shash_hash(void * k) {
	return *((uint32_t *) k);
}

static int as_hashmap_shash_clear(void * key, void * data, void * udata) {
	as_pair * pair = *((as_pair **) data);
	as_pair_destroy(pair);
	return SHASH_REDUCE_DELETE;
}

static int as_hashmap_shash_destroy(void * key, void * data, void * udata) {
	as_pair * pair = *((as_pair **) data);
	as_pair_destroy(pair);
	return 0;
}

static int as_hashmap_shash_foreach(void * key, void * data, void * udata) {
	as_hashmap_shash_foreach_context * ctx = (as_hashmap_shash_foreach_context *) udata;
	as_pair * pair = *((as_pair **) data);
	return ctx->callback(as_pair_1(pair), as_pair_2(pair), ctx->udata) ? 0 : 1;
}

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

as_hashmap * as_hashmap_init(as_hashmap * map, uint32_t capacity)
{
	if ( !map ) return map;

	as_map_cons((as_map *) map, false, NULL, &as_hashmap_map_hooks);
	shash_create((shash **) &(map->htable), as_hashmap_shash_hash, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
	return map;
}

as_hashmap * as_hashmap_new(uint32_t capacity)
{
	as_hashmap * map = (as_hashmap *) cf_malloc(sizeof(as_hashmap));
	if ( !map ) return map;

	as_map_cons((as_map *) map, true, NULL, &as_hashmap_map_hooks);
	shash_create((shash **) &(map->htable), as_hashmap_shash_hash, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
	return map;
}

bool as_hashmap_release(as_hashmap * map)
{
	if ( !map ) return false;

	shash_reduce((shash *) map->htable, as_hashmap_shash_destroy, NULL);
	shash_destroy((shash *) map->htable);
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
	if ( !map ) return 0;

	return shash_get_size((shash *) map->htable);
}

/*******************************************************************************
 *	ACCESSOR & MODIFICATION FUNCTIONS
 ******************************************************************************/

int as_hashmap_set(as_hashmap * map, const as_val * k, const as_val * v)
{
	if ( !map ) return SHASH_ERR;

	uint32_t h = as_val_hashcode(k);
	as_pair * p = NULL;

	if ( shash_get((shash *) map->htable, &h, &p) == SHASH_OK ) {
		as_val_destroy((as_val *) p);
		p = NULL;
	}
	p = pair_new(k,v);
	return shash_put((shash *) map->htable, &h, &p);
}

as_val * as_hashmap_get(const as_hashmap * map, const as_val * k)
{
	if ( !map ) return NULL;

	uint32_t h = as_val_hashcode(k);
	as_pair * p = NULL;

	if ( shash_get((shash *) map->htable, &h, &p) != SHASH_OK ) {
		return NULL;
	}
	as_val * v = as_pair_2(p);
	return v;
}

int as_hashmap_clear(as_hashmap * map)
{
	if ( !map ) return -1;

	shash_reduce_delete((shash *) map->htable, as_hashmap_shash_clear, NULL);
	return 0;
}

int as_hashmap_remove(as_hashmap * map, const as_val * k)
{
	if ( !map ) return -1;

	uint32_t h = as_val_hashcode(k);
	shash_delete_lockfree((shash *) map->htable, &h);
	return 0;
}

/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

bool as_hashmap_foreach(const as_hashmap * map, as_map_foreach_callback callback, void * udata)
{
	if ( !map ) return false;

	as_hashmap_shash_foreach_context ctx = {
		.udata = udata,
		.callback = callback
	};
	return shash_reduce((shash *) map->htable, as_hashmap_shash_foreach, &ctx) == 0;
}
