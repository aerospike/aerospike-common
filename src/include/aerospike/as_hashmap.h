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

#include <aerospike/as_map.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	TYPES
 ******************************************************************************/

/**
 *	A hashtable based implementation of `as_map`.
 *
 *	To use the map, you can either initialize a stack allocated map, 
 *	using `as_hashmap_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_hashmap map;
 *	as_hashmap_init(&map, 32);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated map using 
 *	`as_hashmap_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_hashmap * map = as_hashmap_new(32);
 *	~~~~~~~~~~
 *
 *	When you are finished using the map, then you should release the 
 *	map and associated resources, using `as_hashmap_destroy()`:
 *	
 *	~~~~~~~~~~{.c}
 *	as_hashmap_destroy(list);
 *	~~~~~~~~~~
 *
 *
 *	The `as_hashmap` is a subtype of `as_map`. This allows you to alternatively
 *	use `as_map` functions, by typecasting `as_hashmap` to `as_map`.
 *
 *	~~~~~~~~~~{.c}
 *	as_hashmap map;
 *	as_map * l = (as_list *) as_hashmap_init(&map, 32);
 *	as_stringmap_set_int64(l, "a", 1);
 *	as_stringmap_set_int64(l, "b", 2);
 *	as_stringmap_set_int64(l, "c", 3);
 *	as_map_destroy(l);
 *	~~~~~~~~~~
 *	
 *	The `as_stringmap` functions are simplified functions for using string key.
 *	
 *	Each of the `as_map` functions proxy to the `as_hashmap` functions.
 *	So, calling `as_map_destroy()` is equivalent to calling 
 *	`as_hashmap_destroy()`.
 *
 */
typedef struct as_hashmap_s {
	
	/**
	 *	@private
	 *	as_hashmap is an as_map.
	 *	You can cast as_hashmap to as_map.
	 */
	as_map _;

	/**
	 *	Hashtable
	 */
	void * htable;

} as_hashmap;

/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated hashmap.
 *
 *	@param map 			The map to initialize.
 *	@param buckets		The number of hash buckets to allocate.
 *
 *	@return On success, the initialized map. Otherwise NULL.
 */
as_hashmap * as_hashmap_init(as_hashmap * map, uint32_t buckets);

/**
 *	Creates a new map as a hashmap.
 *
 *	@param buckets		The number of hash buckets to allocate.
 *
 *	@return On success, the new map. Otherwise NULL.
 */
as_hashmap * as_hashmap_new(uint32_t buckets);

/**
 *	@private
 *	Release resources allocated to the list.
 *
 *	@param map	The map to release.
 *
 *	@return On success, true. Otherwise false.
 */
bool as_hashmap_release(as_hashmap * map);

/**
 *	Free the map and associated resources.
 *
 *	@param map 	The map to destroy.
 */
void as_hashmap_destroy(as_hashmap * map);

/*******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

/**
 *	The hash value of the map.
 *
 *	@param map 	The list.
 *
 *	@return The hash value of the list.
 */
uint32_t as_hashmap_hashcode(const as_hashmap * map);

/**
 *	Get the number of entries in the map.
 *
 *	@param map 	The map.
 *
 *	@return The number of entries in the map.
 */
uint32_t as_hashmap_size(const as_hashmap * map);

/*******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 ******************************************************************************/

/**
 *	Get the value for specified key.
 *
 *	@param map 		The map.
 *	@param key		The key.
 *
 *	@return The value for the specified key. Otherwise NULL.
 */
as_val * as_hashmap_get(const as_hashmap * map, const as_val * key);

/**
 *	Set the value for specified key.
 *
 *	@param map 		The map.
 *	@param key		The key.
 *	@param val		The value for the given key.
 *
 *	@return 0 on success. Otherwise an error occurred.
 */
int as_hashmap_set(as_hashmap * map, const as_val * key, const as_val * val);

/**
 *	Remove all entries from the map.
 *
 *	@param map		The map.
 *
 *	@return 0 on success. Otherwise an error occurred.
 */
int as_hashmap_clear(as_hashmap * map);

/******************************************************************************
 *	ITERATION FUNCTIONS
 *****************************************************************************/

/**
 *	Call the callback function for each entry in the map.
 *
 *	@param map		The map.
 *	@param callback	The function to call for each entry.
 *	@param udata	User-data to be passed to the callback.
 *	
 *	@return TRUE on success. Otherwise an error occurred.
 */
bool as_hashmap_foreach(const as_hashmap * map, as_map_foreach_callback callback, void * udata);
