/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>

#include <citrusleaf/cf_shash.h>
#include <citrusleaf/cf_alloc.h>

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_val.h>

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static uint32_t         as_hashmap_shash_hash(void *);
static int              as_hashmap_shash_clear(void *, void *, void *);

static bool             as_hashmap_map_destroy(as_map *);
static uint32_t         as_hashmap_map_hashcode(const as_map *);

static uint32_t         as_hashmap_map_size(const as_map *);
static int              as_hashmap_map_set(as_map *, const as_val *, const as_val *);
static as_val *         as_hashmap_map_get(const as_map *, const as_val *);
static int              as_hashmap_map_clear(as_map *);

static bool             as_hashmap_map_foreach(const as_map *, as_map_foreach_callback, void *);
static as_iterator *    as_hashmap_map_iterator_init(const as_map *, as_iterator *);
static as_iterator *    as_hashmap_map_iterator_new(const as_map *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_map_hooks as_hashmap_map_hooks = {
	.destroy        = as_hashmap_map_destroy,
	.hashcode       = as_hashmap_map_hashcode,

	.size           = as_hashmap_map_size,
	.set            = as_hashmap_map_set,
	.get            = as_hashmap_map_get,
	.clear          = as_hashmap_map_clear,
	
	.foreach        = as_hashmap_map_foreach,
	.iterator_init  = as_hashmap_map_iterator_init,
	.iterator_new   = as_hashmap_map_iterator_new
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_map * as_hashmap_init(as_map * m, uint32_t capacity)
{
	as_val_init(&m->_, AS_MAP, false);
	m->hooks = &as_hashmap_map_hooks;
	shash_create((shash **) &(m->data.hashmap.htable), as_hashmap_shash_hash, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
	return m;
}

as_map * as_hashmap_new(uint32_t capacity)
{
	as_map *m = (as_map *) malloc(sizeof(as_map));
	as_val_init(&m->_, AS_MAP, true);
	m->hooks = &as_hashmap_map_hooks;
	shash_create((shash **) &(m->data.hashmap.htable), as_hashmap_shash_hash, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
	return m;
}

void as_hashmap_destroy(as_map * m)
{
	as_val_val_destroy( (as_val *) m );
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static uint32_t as_hashmap_shash_hash(void * k)
{
	return *((uint32_t *) k);
}

static int as_hashmap_shash_clear(void * key, void * data, void * udata)
{
	as_pair * pair = *((as_pair **) data);
	as_pair_destroy(pair);
	return SHASH_REDUCE_DELETE;
}

static int as_hashmap_shash_destroy(void * key, void * data, void * udata)
{
	as_pair * pair = *((as_pair **) data);
	as_pair_destroy(pair);
	return 0;
}

typedef struct {
	void *                  udata;
	as_map_foreach_callback callback;
} as_hashmap_shash_foreach_context;

static int as_hashmap_shash_foreach(void * key, void * data, void * udata)
{
	as_hashmap_shash_foreach_context * ctx = (as_hashmap_shash_foreach_context *) udata;
	as_pair * pair = *((as_pair **) data);
	return ctx->callback(as_pair_1(pair), as_pair_2(pair), ctx->udata);
}

static bool as_hashmap_map_destroy(as_map * m)
{
	as_hashmap * s = &(m->data.hashmap);
	shash_reduce((shash *) s->htable, as_hashmap_shash_destroy, NULL);
	shash_destroy((shash *) s->htable);
	return true;
}

/**
 * @TODO fillout with appropriate algo.
 */
static uint32_t as_hashmap_map_hashcode(const as_map *m)
{
	return 1;
}

/**
 * Set the value for the given key
 */
static int as_hashmap_map_set(as_map * m, const as_val * k, const as_val * v)
{

	as_hashmap * s = &(m->data.hashmap);
	uint32_t h = as_val_hashcode(k);
	as_pair * p = NULL;

	if ( shash_get((shash *) s->htable, &h, &p) == SHASH_OK ) {
		as_val_destroy((as_val *)p);
		p = NULL;
	}
	p = pair_new(k,v);
	return shash_put((shash *) s->htable, &h, &p);
}

/**
 * Get the value for the given key
 */
static as_val * as_hashmap_map_get(const as_map * m, const as_val * k)
{
	const as_hashmap * s = &(m->data.hashmap);
	uint32_t h = as_val_hashcode(k);
	as_pair * p = NULL;

	if ( shash_get((shash *) s->htable, &h, &p) != SHASH_OK ) {
		return NULL;
	}
	as_val *v = as_pair_2(p);
	return v;
}

/**
 * Get the number of entries in the map.
 */
static uint32_t as_hashmap_map_size(const as_map * m)
{
	const as_hashmap * s = &(m->data.hashmap);
	return shash_get_size((shash *) s->htable);
}

/**
 * Removes all entries from the map.
 */
static int as_hashmap_map_clear(as_map * m)
{
	as_hashmap * s = &(m->data.hashmap);
	shash_reduce_delete((shash *) s->htable, as_hashmap_shash_clear, NULL);
	return 0;
}

/**
 * Calls the callback function for each (key,value) pair in the map.
 */
static bool as_hashmap_map_foreach(const as_map * m, as_map_foreach_callback callback, void * udata)
{
	as_hashmap * s = (as_hashmap *) &(m->data.hashmap);
	as_hashmap_shash_foreach_context ctx = {
		.udata = udata,
		.callback = callback
	};
	return shash_reduce_delete((shash *) s->htable, as_hashmap_shash_foreach, &ctx) == TRUE;
}

/**
 * Initializes an iterator over the (key,value) pairs in the map.
 */
static as_iterator * as_hashmap_map_iterator_init(const as_map * m, as_iterator * i)
{
	return as_hashmap_iterator_init(&m->data.hashmap, i);
}

/**
 * Creates a new iterator over the (key,value) pairs in the map.
 */
static as_iterator * as_hashmap_map_iterator_new(const as_map * m)
{
	return as_hashmap_iterator_new(&m->data.hashmap);
}
