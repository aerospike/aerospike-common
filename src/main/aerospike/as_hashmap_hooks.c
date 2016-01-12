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

/*******************************************************************************
 *	EXTERN FUNCTIONS
 ******************************************************************************/

extern bool as_hashmap_release(as_hashmap * map);

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

static int _as_hashmap_map_remove(as_map * m, const as_val * k)
{
	return as_hashmap_remove((as_hashmap *) m, k);
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
	.remove		= _as_hashmap_map_remove,
	
	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/

	.foreach		= _as_hashmap_map_foreach,
	.iterator_new	= _as_hashmap_map_iterator_new,
	.iterator_init	= _as_hashmap_map_iterator_init,

};
