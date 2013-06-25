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

#pragma once

/******************************************************************************
 *	TYPES
 ******************************************************************************/

struct as_map_s;

/**
 *	A hashmap implementation for `as_map`.
 *
 *	To use the map, you can either initialize a stack allocated map, 
 *	using `as_hashmap_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_map map;
 *	as_hashmap_init(&map, 32);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated map using `as_hashmap_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_map * map = as_hashmap_new(32);
 *	~~~~~~~~~~
 *
 *	When you are finished using the map, then you should release the map and
 *	associated resources, using `as_map_destroy()`:
 *	
 *	~~~~~~~~~~{.c}
 *	as_map_destroy(map);
 *	~~~~~~~~~~
 *
 *	@see `as_map`
 */
typedef struct as_hashmap_s {

	/**
	 *	Hashtable value
	 */
	void * htable;

} as_hashmap;

/******************************************************************************
 *	FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated `as_map` as an `as_hashmap`.
 *
 *	@param map		The map to initialize.
 *	@param buckets	The number of hash buckets to allocate.
 *
 *	@return On success, the initialized map. Otherwise NULL.
 */
struct as_map_s * as_hashmap_init(struct as_map_s * map, uint32_t buckets);

/**
 *	Creates a new heap allocated `as_map` as an `as_hashmap`.
 *
 *	@param buckets	The number of hash buckets to allocate.
 *
 *	@return On success, the initialized map. Otherwise NULL.
 */
struct as_map_s * as_hashmap_new(uint32_t buckets);

/**
 *	Free the map and associated resources.
 *
 *	@param map		The map to destroy.
 */
void as_hashmap_destroy(struct as_map_s * map); 
