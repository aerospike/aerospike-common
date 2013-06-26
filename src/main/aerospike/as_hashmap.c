/******************************************************************************
 *	Copyright 2008-2013 by Aerospike.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy 
 *	of this software and associated documentation files (the "Software"), to 
 *	deal in the Software without restriction, including without limitation the 
 *	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 *	sell copies of the Software, and to permit persons to whom the Software is 
 *	furnished to do so, subject to the following conditions:
 * 
 *	The above copyright notice and this permission notice shall be included in 
 *	all copies or substantial portions of the Software.
 * 
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *	IN THE SOFTWARE.
 *****************************************************************************/

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_val.h>

#include <citrusleaf/cf_shash.h>

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
	return ctx->callback(as_pair_1(pair), as_pair_2(pair), ctx->udata);
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
	as_hashmap * map = (as_hashmap *) malloc(sizeof(as_hashmap));
	if ( !map ) return map;

	as_map_cons((as_map *) map, true, NULL, &as_hashmap_map_hooks);
	shash_create((shash **) &(map->htable), as_hashmap_shash_hash, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
	return map;
}

bool as_hashmap_release(as_hashmap * map)
{
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
	return shash_get_size((shash *) map->htable);
}

/*******************************************************************************
 *	ACCESSOR & MODIFICATION FUNCTIONS
 ******************************************************************************/

int as_hashmap_set(as_hashmap * map, const as_val * k, const as_val * v)
{
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
	shash_reduce_delete((shash *) map->htable, as_hashmap_shash_clear, NULL);
	return 0;
}

/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

bool as_hashmap_foreach(const as_hashmap * map, as_map_foreach_callback callback, void * udata)
{
	as_hashmap_shash_foreach_context ctx = {
		.udata = udata,
		.callback = callback
	};
	return shash_reduce((shash *) map->htable, as_hashmap_shash_foreach, &ctx) == TRUE;
}
