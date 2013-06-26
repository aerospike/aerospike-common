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
#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

union as_map_iterator_u;

struct as_map_hooks_s;

/**
 *	Callback function for `as_map_foreach()`. Called for each entry in the 
 *	map.
 *	
 *	@param key 		The key of the current entry.
 *	@param value 	The value of the current entry.
 *	@param udata	The user-data provided to the `as_list_foreach()`.
 *	
 *	@return true to continue iterating through the list. 
 *			false to stop iterating.
 */
typedef bool (* as_map_foreach_callback) (const as_val * key, const as_val * value, void * udata);

/**
 *	Map base type.
 *
 *	This is the base type for all map types.
 *
 *	The map struct should not be used directly.
 *
 *	@see `as_hashmap` for 
 */
typedef struct as_map_s {

	/**
	 *	@private
	 *	as_map is a subtype of as_val.
	 *	You can cast as_map to as_val.
	 */
	as_val _;

	/**
	 *	Pointer to the data for this list.
	 */
	void * data;

	/**
	 * Hooks for sybtypes of as_list to implement.
	 */
	const struct as_map_hooks_s * hooks;

} as_map;

/**
 *	Map Function Hooks
 */
typedef struct as_map_hooks_s {

	/***************************************************************************
	 *	instance hooks
	 **************************************************************************/

	/**
	 *	Releases the subtype of as_map.
	 *
	 *	@param map 	The map instance to destroy.
	 *
	 *	@return true on success. Otherwise false.
	 */
	bool (* destroy)(as_map * map);

	/***************************************************************************
	 *	info hooks
	 **************************************************************************/

	/**
	 *	The hash value of an as_map.
	 *
	 *	@param map	The map to get the hashcode value for.
	 *
	 *	@retun The hashcode value.
	 */
	uint32_t (* hashcode)(const as_map * map);
	
	/**
	 *	The size of the as_map.
	 *
	 *	@param map	The map to get the size of.
	 *
	 *	@return The number of entries in the map.
	 */
	uint32_t (* size)(const as_map * map);

	/***************************************************************************
	 *	accessor and modifier hooks
	 **************************************************************************/

	/**
	 *	Set a value of the given key in a map.
	 *
	 *	@param map 	The map to store the (key,value) pair.
	 *	@param key 	The key for the given value.
	 *	@param val 	The value for the given key.
	 *
	 *	@return 0 on success. Otherwise an error occurred.
	 */
	int (* set)(as_map * map, const as_val * key, const as_val * val);

	/**
	 *	Set a value at the given key of the map.
	 *
	 *	@param map 	The map to containing the (key,value) pair.
	 *	@param key 	The key of the value.
	 *
	 *	@return The value on success. Otherwise NULL.
	 */
	as_val * (* get)(const as_map * map, const as_val * key);

	/**
	 *	Clear all entries of the map.
	 *
	 *	@param map 	The map to clear.
	 *
	 *	@return 0 on success. Otherwise an error occurred.
	 */
	int (* clear)(as_map * map);

	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/
	
	/**
	 *	Iterate over each entry in the map can call the callback function.
	 *
	 *	@param map 		The map to iterate.
	 *	@param callback	The function to call for each entry in the map.
	 *	@param udata 	User-data to be passed to the callback.
	 *
	 *	@return true on success. Otherwise false.
	 */
	bool (* foreach)(const as_map * map, as_map_foreach_callback callback, void * udata);

	/**
	 *	Create and initialize a new heap allocated iterator to traverse over the entries map.
	 *
	 *	@param map 	The map to iterate.
	 *	
	 *	@return true on success. Otherwise false.
	 */
	union as_map_iterator_u * (* iterator_new)(const as_map * map);

	/**
	 *	Initialize a stack allocated iterator to traverse over the entries map.
	 *
	 *	@param map 	The map to iterate.
	 *	
	 *	@return true on success. Otherwise false.
	 */
	union as_map_iterator_u * (* iterator_init)(const as_map * map, union as_map_iterator_u * it);

} as_map_hooks;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Utilized by subtypes of as_map to initialize the parent.
 *
 *	@param map		The map to initialize
 *	@param free 	If TRUE, then as_map_destory() will free the map.
 *	@param data		Data for the map.
 *	@param hooks	Implementaton for the map interface.
 *	
 *	@return The initialized as_map on success. Otherwise NULL.
 */
as_map * as_map_cons(as_map * map, bool free, void * data, const as_map_hooks * hooks);

/**
 *	Initialize a stack allocated map.
 *
 *	@param map		Stack allocated map to initialize.
 *	@param data		Data for the map.
 *	@param hooks	Implementaton for the map interface.
 *	
 *	@return On success, the initialized map. Otherwise NULL.
 */
as_map * as_map_init(as_map * map, void * data, const as_map_hooks * hooks);

/**
 *	Create and initialize a new heap allocated map.
 *	
 *	@param data		Data for the list.
 *	@param hooks	Implementaton for the list interface.
 *	
 *	@return On succes, a new list. Otherwise NULL.
 */
as_map * as_map_new(void * data, const as_map_hooks * hooks);

/**
 *	Destroy the as_map and associated resources.
 */
inline void as_map_destroy(as_map * map) 
{
	as_val_destroy((as_val *) map);
}

/*******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

/**
 *	Hash value for the map
 *
 *	@param map		The map
 *
 *	@return The hashcode value of the map.
 */
inline uint32_t as_map_hashcode(const as_map * map) 
{
	return as_util_hook(hashcode, 0, map);
}

/**
 *	Get the number of entries in the map.
 *
 *	@param map		The map
 *
 *	@return The size of the map.
 */
inline uint32_t as_map_size(const as_map * map) 
{
	return as_util_hook(size, 0, map);
}

/*******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 ******************************************************************************/

/**
 *	Get the value for specified key.
 *
 *	@param map		The map.
 *	@param key		The key.
 *
 *	@return The value for the specified key on success. Otherwise NULL.
 */
inline as_val * as_map_get(const as_map * map, const as_val * key)
{
	return as_util_hook(get, NULL, map, key);
}

/**
 *	Set the value for specified key.
 *
 *	@param map		The map.
 *	@param key		The key.
 *	@param val		The value for the key.
 *	
 *	@return 0 on success. Otherwise an error occurred.
 */
inline int as_map_set(as_map * map, const as_val * key, const as_val * val) 
{
	return as_util_hook(set, 1, map, key, val);
}

/**
 *	Remove all entries from the map.
 *
 *	@param map		The map.
 *
 *	@return 0 on success. Otherwise an error occurred.
 */
inline int as_map_clear(as_map * map)
{
	return as_util_hook(clear, 1, map);
}

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
inline bool as_map_foreach(const as_map * map, as_map_foreach_callback callback, void * udata) 
{
	return as_util_hook(foreach, false, map, callback, udata);
}

/**
 *	Creates and initializes a new heap allocated iterator over the given map.
 *
 *	@param map 	The map to iterate.
 *
 *	@return On success, a new as_iterator. Otherwise NULL.
 */
inline union as_map_iterator_u * as_map_iterator_new(const as_map * map) 
{
	return as_util_hook(iterator_new, NULL, map);
}

/**
 *	Initialzies a stack allocated iterator over the given map.
 *
 *	@param map 	The map to iterate.
 *	@param it 	The iterator to initialize.
 *
 *	@return On success, the initializes as_iterator. Otherwise NULL.
 */
inline union as_map_iterator_u * as_map_iterator_init(union as_map_iterator_u * it, const as_map * map) 
{
	return as_util_hook(iterator_init, NULL, map, it);
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_map_toval(const as_map * map) 
{
	return (as_val *) map;
}

/**
 *	Convert from an as_val.
 */
inline as_map * as_map_fromval(const as_val * val) 
{
	return as_util_fromval(val, AS_MAP, as_map);
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_map_val_destroy(as_val * val);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_map_val_hashcode(const as_val * val);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_map_val_tostring(const as_val * val);
