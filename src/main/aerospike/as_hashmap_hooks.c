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
#include <aerospike/as_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_map_iterator.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_val.h>

#include <citrusleaf/cf_shash.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "internal.h"

/*******************************************************************************
 *	FUNCTIONS
 ******************************************************************************/

static bool _as_hashmap_map_destroy(as_map * m) 
{
	return as_hashmap_release((as_hashmap *) m);
}

static uint32_t _as_hashmap_map_hashcode(const as_map * m)
{
	return as_hashmap_hashcode((const as_hashmap *) m);
}

static int _as_hashmap_map_set(as_map * m, const as_val * k, const as_val * v)
{
	return as_hashmap_set((as_hashmap *) m, k, v);
}

static as_val * _as_hashmap_map_get(const as_map * m, const as_val * k)
{
	return as_hashmap_get((as_hashmap *) m, k);
}

static uint32_t _as_hashmap_map_size(const as_map * m)
{
	return as_hashmap_size((const as_hashmap *) m);
}

static int _as_hashmap_map_clear(as_map * m)
{
	return as_hashmap_clear((as_hashmap *) m);
}

static bool _as_hashmap_map_foreach(const as_map * m, as_map_foreach_callback callback, void * udata) 
{
	return as_hashmap_foreach((const as_hashmap *) m, callback, udata);
}

static as_map_iterator * _as_hashmap_map_iterator_new(const as_map * m) 
{
	return (as_map_iterator *) as_hashmap_iterator_new((const as_hashmap *) m);
}

static as_map_iterator * _as_hashmap_map_iterator_init(const as_map * m, as_map_iterator * it)
{
	return (as_map_iterator *) as_hashmap_iterator_init((as_hashmap_iterator *) it, (as_hashmap *) m);
}

/*******************************************************************************
 *	HOOKS
 ******************************************************************************/

const as_map_hooks as_hashmap_map_hooks = {

	/***************************************************************************
	 *	instance hooks
	 **************************************************************************/

	.destroy	= _as_hashmap_map_destroy,

	/***************************************************************************
	 *	info hooks
	 **************************************************************************/

	.hashcode	= _as_hashmap_map_hashcode,
	.size		= _as_hashmap_map_size,

	/***************************************************************************
	 *	accessor and modifier hooks
	 **************************************************************************/

	.set		= _as_hashmap_map_set,
	.get		= _as_hashmap_map_get,
	.clear		= _as_hashmap_map_clear,
	
	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/

	.foreach		= _as_hashmap_map_foreach,
	.iterator_new	= _as_hashmap_map_iterator_new,
	.iterator_init	= _as_hashmap_map_iterator_init,

};
