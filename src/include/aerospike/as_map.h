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

#include <aerospike/as_iterator.h>
#include <aerospike/as_hashmap.h>
#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

struct as_map_hooks_s;

/**
 *	Callback function for as_map_foreach()
 */
typedef bool (* as_map_foreach_callback) (const as_val *, const as_val *, void *);

/**
 *	Map Data
 */
typedef union as_map_data_u {
	as_hashmap  hashmap;
	void *      generic;
} as_map_data;

/**
 *	Map Interface.
 *
 *	To use the map interface, you will need to create an instance 
 *	via one of the implementations.
 */
typedef struct as_map_s {

	/**
	 *	@private
	 *	as_boolean is a subtype of as_val.
	 *	You can cast as_boolean to as_val.
	 */
	as_val _;
	
	/**
	 *	Data provided by the implementation of `as_map`.
	 */
	as_map_data data;
	
	/**
	 *	Hooks provided by the implementation of `as_map`.
	 */
	const struct as_map_hooks_s * hooks;

} as_map;

/**
 *	Map Hooks
 *
 *	An implementation of `as_map` should provide implementations for each
 *	of the hooks.
 */
typedef struct as_map_hooks_s {

	/**
	 *	Destroy the map.
	 *
	 *	@retun true on success. Otherwise false.
	 */
	bool (* destroy)(as_map * map);

	/**
	 *	The hashcode for the map.
	 */
	uint32_t (* hashcode)(const as_map * map);

	/**
	 *	The number of entries in the map.
	 */
	uint32_t (* size)(const as_map * map);

	/**
	 *	Set the value of the specified key of the map.
	 *
	 *	@param map		The map.
	 *	@param key		They key to set the value for.
	 *	@param val		They value for the key.
	 *
	 *	@return On success, 0. Otherwise an error occurred.
	 */
	int (* set)(as_map * map, const as_val * key, const as_val * val);

	/**
	 *	Get the value of the specified key of the map.
	 *
	 *	@param map		The map.
	 *	@param key		The key to get the value for.
	 *
	 *	@param On success, the value. Otherwise NULL.
	 */
	as_val * (* get)(const as_map * map, const as_val * key);

	/**
	 *	Remove all entries from the map.
	 *
	 *	@return On success, 0. Otherwise an error occurred.
	 */
	int (* clear)(as_map * map);

	/**
	 *	Iterate over each entry of the map and call the callback
	 *	function. The udata is sent to the callback function.
	 *
	 *	@param map		The map to iterate.
	 *	@param callback	The function to call for each entry in the map.
	 *	@param udata	User-data to be passed to the callback function.
	 *
	 *	@return		true, to continue the iteration. 
	 *				false, to stop the iteration.
	 */
	bool (* foreach)(const as_map *, as_map_foreach_callback, void *);

	/**
	 *	Initialize an iterator for the map.
	 *
	 *	@param map		The map to iterate.
	 *	@param iterator	The iterator to initialize.
	 *
	 *	@return On success, the initialized iterator. Otherwise NULL.
	 */
	as_iterator * (* iterator_init)(const as_map *, as_iterator *);

	/**
	 *	Create a new iterator for the list.
	 *
	 *	@param map		The map to iterate.
	 *	
	 *	@return On success, a new iterator. Otherwise NULL.
	 */
	as_iterator * (* iterator_new)(const as_map *);

} as_map_hooks;

/******************************************************************************
 *	INLINE FUNCTIONS
 *****************************************************************************/

/**
 *	Destroy the as_map and associated resources.
 */
inline void as_map_destroy(as_map * m) 
{
	as_val_val_destroy((as_val *) m);
}

/**
 *	Get the number of entries in the map.
 */
inline uint32_t as_map_size(const as_map * m) 
{
	return as_util_hook(size, 0, m);
}

/**
 *	Get the value for specified key.
 */
inline as_val * as_map_get(const as_map * m, const as_val * k)
{
	return as_util_hook(get, NULL, m, k);
}

/**
 *	Set the value for specified key.
 */
inline int as_map_set(as_map * m, const as_val * k, const as_val * v) 
{
	return as_util_hook(set, 1, m, k, v);
}

/**
 *	Remove all entries from the map.
 */
inline int as_map_clear(as_map * m) {
	return as_util_hook(clear, 1, m);
}

/******************************************************************************
 *	ITERATION FUNCTIONS
 *****************************************************************************/

/**
 *	Call the callback function for each entry in the map.
 */
inline void as_map_foreach(const as_map * m, as_map_foreach_callback callback, void * udata) 
{
	as_util_hook(foreach, false, m, callback, udata);
}

/**
 *	Initializes a stack allocated iterator to iterate over the entries in the map.
 */
inline as_iterator * as_map_iterator_init(as_iterator *i, const as_map * m) 
{
	return as_util_hook(iterator_init, NULL, m, i);
}

/**
 *	Create a new heap allocated iterator to iterate over the entries in the map.
 */
inline as_iterator * as_map_iterator_new(const as_map * m) 
{
	return as_util_hook(iterator_new, NULL, m);
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_map_toval(const as_map * m) 
{
	return (as_val *) m;
}

/**
 *	Convert from an as_val.
 */
inline as_map * as_map_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_MAP, as_map);
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_map_val_destroy(as_val * v);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_map_val_hashcode(const as_val * v);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_map_val_tostring(const as_val * v);
