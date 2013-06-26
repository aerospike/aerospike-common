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

#include <aerospike/as_bytes.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_string.h>
#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

union as_list_iterator_u;

struct as_list_hooks_s;

/**
 *	Callback function for `as_list_foreach()`. Called for each element in the 
 *	list.
 *	
 *	@param value 	The value of the current element.
 *	@param udata	The user-data provided to the `as_list_foreach()`.
 *	
 *	@return true to continue iterating through the list. 
 *			false to stop iterating.
 */
typedef bool (* as_list_foreach_callback) (as_val * value, void * udata);

/**
 *	List base type.
 *
 *	This is the base type for all list types.
 *
 *	The list struct should not be used directly.
 */
typedef struct as_list_s {

	/**
	 *	@private
	 *	as_list is a subtype of as_val.
	 *	You can cast as_list to as_val.
	 */
	as_val _;

	/**
	 *	Pointer to the data for this list.
	 */
	void * data;

	/**
	 * Hooks for subtypes of as_list to implement.
	 */
	const struct as_list_hooks_s * hooks;

} as_list;

/**
 *	List Function Hooks
 */
typedef struct as_list_hooks_s {

	/***************************************************************************
	 *	instance hooks
	 **************************************************************************/

	/**
	 *	Releases the subtype of as_list.
	 *
	 *	@param map 	The map instance to destroy.
	 *
	 *	@return true on success. Otherwise false.
	 */
	bool (* destroy)(as_list * list);

	/***************************************************************************
	 *	info hooks
	 **************************************************************************/

	/**
	 *	The hash value of an as_list.
	 *
	 *	@param list	The list to get the hashcode value for.
	 *
	 *	@retun The hashcode value.
	 */
	uint32_t (* hashcode)(const as_list * list);

	/**
	 *	The size of the as_list.
	 *
	 *	@param map	The map to get the size of.
	 *
	 *	@return The number of entries in the map.
	 */
	uint32_t (* size)(const as_list * list);

	/***************************************************************************
	 *	accessor and modifier hooks
	 **************************************************************************/

	/**
	 *	Append the value to the list.
	 *
	 *	@param list		The list to append to.
	 *	@param value	The value to append to the list.
	 *
	 *	@return 0 on success. Otherwise an error occurred.
	 */
	int (* append)(as_list * list, as_val * value);

	/**
	 *	Prepend the value to the list.
	 *
	 *	@param list		The list to prepend to.
	 *	@param value	The value to prepend to the list.
	 *
	 *	@return 0 on success. Otherwise an error occurred.
	 */
	int (* prepend)(as_list * list, as_val * value);

	/**
	 *	Get the value at a given index of the list.
	 *
	 *	@param list 	The list to get the value from.
	 *	@param index 	The index of the value.
	 *
	 *	@return The value at the given index on success. Otherwie NULL.
	 */
	as_val * (* get)(const as_list * list, const uint32_t index);

	/**
	 *	Set a value at the given index of the list.
	 *
	 *	@param list 	The list to get the value from.
	 *	@param index 	The index of the value.
	 *	@param value 	The value for the given index.
	 *
	 *	@return The value at the given index on success. Otherwie NULL.
	 */
	int (* set)(as_list * list, const uint32_t index, as_val * value);

	/**
	 *	Return the first element in the list.
	 *
	 *	@param list 	The list to get the value from.
	 *
	 *	@return The first value in the list. Otherwise NULL.
	 */
	as_val * (* head)(const as_list * list);

	/**
	 *	Return all but the first element of the list, returning a new list.
	 *
	 *	@param list 	The list to get the list from.
	 *
	 *	@return The tail of the list. Otherwise NULL.
	 */
	as_list * (* tail)(const as_list * list);

	/**
	 *	Drop the first n element of the list, returning a new list.
	 *
	 *	@param list 	The list.
	 *	@param n 		The number of element to drop.
	 *
	 *	@return A new list containing the remaining elements. Otherwise NULL.
	 */
	as_list * (* drop)(const as_list * list, uint32_t n);

	/**
	 *	Take the first n element of the list, returning a new list.
	 *
	 *	@param list 	The list.
	 *	@param n 		The number of element to take.
	 *	
	 *	@return A new list containing the remaining elements. Otherwise NULL.
	 */
	as_list * (* take)(const as_list * list, uint32_t n);

	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/

	/**
	 *	Iterate over each element in the list can call the callback function.
	 *
	 *	@param map 		The map to iterate.
	 *	@param callback	The function to call for each element in the list.
	 *	@param udata 	User-data to be passed to the callback.
	 *
	 *	@return true on success. Otherwise false.
	 */
	bool (* foreach)(const as_list * list, as_list_foreach_callback callback, void * udata);

	/**
	 *	Create and initialize a new heap allocated iterator to traverse over the list.
	 *
	 *	@param list 	The list to iterate.
	 *	
	 *	@return true on success. Otherwise false.
	 */
	union as_list_iterator_u * (* iterator_new)(const as_list * list);

	/**
	 *	Initializes a stack allocated iterator to traverse over the list.
	 *
	 *	@param list 	The list to iterate.
	 *	
	 *	@return true on success. Otherwise false.
	 */
	union as_list_iterator_u * (* iterator_init)(const as_list * list, union as_list_iterator_u * it);

} as_list_hooks;

/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	@private
 *	Utilized by subtypes of as_list to initialize the parent.
 *
 *	@param list		The list to initialize.
 *	@param free		If true, then as_list_destroy() will free the list.
 *	@param data		Data for the list.
 *	@param hooks	Implementaton for the list interface.
 *
 *	@return On success, the initialized list. Otherwise NULL.
 */
as_list * as_list_cons(as_list * list, bool free, void * data, const as_list_hooks * hooks);

/**
 *	Initialize a stack allocated list.
 *	
 *	@param list		Stack allocated list to initialize.
 *	@param data		Data for the list.
 *	@param hooks	Implementaton for the list interface.
 *	
 *	@return On succes, the initialized list. Otherwise NULL.
 */
as_list * as_list_init(as_list * list, void * data, const as_list_hooks * hooks);

/**
 *	Create and initialize a new heap allocated list.
 *	
 *	@param data		Data for the list.
 *	@param hooks	Implementaton for the list interface.
 *	
 *	@return On succes, a new list. Otherwise NULL.
 */
as_list * as_list_new(void * data, const as_list_hooks * hooks);

/**
 *	Destroy the list and associated resources.
 *
 *	@param list 	The list to destroy.
 */
inline void as_list_destroy(as_list * list) 
{
	as_val_destroy((as_val *) list);
}

/******************************************************************************
 *	INFO FUNCTIONS
 *****************************************************************************/

/**
 *	Get the hashcode value for the list.
 *
 *	@param list 	The list.
 *
 *	@return The hashcode of the list.
 */
inline uint32_t as_list_hashcode(as_list * list) 
{
	return as_util_hook(hashcode, 0, list);
}

/**
 *	Number of elements in the list.
 *
 *	@param list 	The list.
 *
 *	@return The size of the list.
 */
inline uint32_t as_list_size(as_list * list) 
{
	return as_util_hook(size, 0, list);
}

/******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 *****************************************************************************/

/**
 *	The first element in the list.
 *
 *	@param list		The list to get the head value from.
 *
 *	@return The first value of the list on success. Otherwise NULL.
 */
inline as_val * as_list_head(const as_list * list) 
{
	return as_util_hook(head, NULL, list);
}

/**
 *	All elements after the first element in the list.
 *
 *	@param list		The list to get the tail from.
 *
 *	@return On success, the tail of the list. Otherwise NULL.
 */
inline as_list * as_list_tail(const as_list * list) 
{
	return as_util_hook(tail, NULL, list);
}

/**
 *	Create a new list containing all elements, except the first n elements, of the list.
 *
 *	@param list 	The list to drop elements from.
 *	@param n		The number of elements to drop.
 *
 *	@return On success, a new list containing the remaining elements. Otherwise NULL.
 */
inline as_list * as_list_drop(const as_list * list, uint32_t n) 
{
	return as_util_hook(drop, NULL, list, n);
}

/**
 *	Creates a new list containing the first n elements of the list.
 *
 *	@param list 	The list to drop elements from.
 *	@param n		The number of elements to take.
 *
 *	@return On success, a new list containing the selected elements. Otherwise NULL.
 */
inline as_list * as_list_take(const as_list * list, uint32_t n) 
{
	return as_util_hook(take, NULL, list, n);
}

/******************************************************************************
 *	GETTER FUNCTIONS
 *****************************************************************************/

/**
 *	Get the value at specified index as an as_val.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline as_val * as_list_get(const as_list * list, const uint32_t i) 
{
	return as_util_hook(get, NULL, list, i);
}

/**
 *	Get the value at specified index as an int64_t.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline int64_t as_list_get_int64(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	as_integer * iv = as_integer_fromval(v);
	return iv ? as_integer_toint(iv) : 0;
}

/**
 *	Get the value at specified index as an NULL terminated string.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline char * as_list_get_str(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	as_string * sv = as_string_fromval(v);
	return sv ? as_string_tostring(sv) : NULL;
}

/**
 *	Get the value at specified index as an as_integer.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline as_integer * as_list_get_integer(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	return v && v->type == AS_INTEGER ? (as_integer *) v : NULL;
}

/**
 *	Get the value at specified index as an as_val.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline as_string * as_list_get_string(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	return v && v->type == AS_STRING ? (as_string *) v : NULL;
}

/**
 *	Get the value at specified index as an as_val.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline as_bytes * as_list_get_bytes(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	return v && v->type == AS_BYTES ? (as_bytes *) v : NULL;
}

/**
 *	Get the value at specified index as an as_val.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline as_list * as_list_get_list(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	return v && v->type == AS_LIST ? (as_list *) v : NULL;
}

/**
 *	Get the value at specified index as an as_val.
 *
 *	@param list		The list to get the value from.
 *	@param i		The index of the value to get from the list.
 *
 *	@return On success, the value at the given index. Otherwise NULL.
 */
inline struct as_map_s * as_list_get_map(const as_list * list, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, list, i);
	return v && v->type == AS_MAP ? (struct as_map_s *) v : NULL;
}


/******************************************************************************
 *	SETTER FUNCTIONS
 *****************************************************************************/

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set(as_list * list, const uint32_t i, as_val * value) 
{
	return as_util_hook(set, 1, list, i, value);
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_int64(as_list * list, const uint32_t i, int64_t value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) as_integer_new(i));
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_str(as_list * list, const uint32_t i, const char * value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) as_string_new(strdup(value), true));
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_integer(as_list * list, const uint32_t i, as_integer * value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) value);
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_string(as_list * list, const uint32_t i, as_string * value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) value);
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_bytes(as_list * list, const uint32_t i, as_bytes * value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) value);
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_list(as_list * list, const uint32_t i, as_list * value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) value);
}

/**
 *	Set the value at specified index as an as_val.
 *
 *	@param list		The list.
 *	@param i		The index of the value to set in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_set_map(as_list * list, const uint32_t i, struct as_map_s * value) 
{
	return as_util_hook(set, 1, list, i, (as_val *) value);
}

/******************************************************************************
 *	APPEND FUNCTIONS
 *****************************************************************************/

/**
 *	Append an as_val to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append(as_list * list, as_val * value) 
{
	return as_util_hook(append, 1, list, value);
}

/**
 *	Append an int64_t to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_int64(as_list * list, int64_t value) 
{
	return as_util_hook(append, 1, list, (as_val *) as_integer_new(value));
}

/**
 *	Append a NULL terminates string to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_str(as_list * list, const char * value) 
{
	return as_util_hook(append, 1, list, (as_val *) as_string_new(strdup(value), true));
}

/**
 *	Append an as_integer to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_integer(as_list * list, as_integer * value) 
{
	return as_util_hook(append, 1, list, (as_val *) value);
}

/**
 *	Append an as_string to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_string(as_list * list, as_string * value) 
{
	return as_util_hook(append, 1, list, (as_val *) value);
}

/**
 *	Append an as_bytes to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_bytes(as_list * list, as_bytes * value) 
{
	return as_util_hook(append, 1, list, (as_val *) value);
}

/**
 *	Append an as_list to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_list(as_list * list, as_list * value) 
{
	return as_util_hook(append, 1, list, (as_val *) value);
}

/**
 *	Append an as_map to the list.
 *
 *	@param list		The list.
 *	@param value	The value to append to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_append_map(as_list * list, struct as_map_s * value) 
{
	return as_util_hook(append, 1, list, (as_val *) value);
}

/******************************************************************************
 *	PREPEND FUNCTIONS
 *****************************************************************************/

/**
 *	Prepend an as_val to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend(as_list * list, as_val * v) 
{
	return as_util_hook(prepend, 1, list, v);
}

/**
 *	Prepend an int64_t to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_int64(as_list * list, int64_t value) 
{
	return as_util_hook(prepend, 1, list, (as_val *) as_integer_new(value));
}

/**
 *	Prepend a NULL terminates string to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_str(as_list * list, const char * value) 
{
	return as_util_hook(prepend, 1, list, (as_val *) as_string_new(strdup(value), true));
}

/**
 *	Prepend an as_integer to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_integer(as_list * list, as_integer * value) 
{
	return as_util_hook(prepend, 1, list, (as_val *) value);
}

/**
 *	Prepend an as_string to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_string(as_list * list, as_string * value) 
{
	return as_util_hook(prepend, 1, list, (as_val *) value);
}

/**
 *	Prepend an as_bytes to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_bytes(as_list * list, as_bytes * value)
{
	return as_util_hook(prepend, 1, list, (as_val *) value);
}

/**
 *	Prepend an as_list to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_list(as_list * list, as_list * value) 
{
	return as_util_hook(prepend, 1, list, (as_val *) value);
}

/**
 *	Prepend an as_map to the list.
 *
 *	@param list		The list.
 *	@param value	The value to prepend to the list.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline int as_list_prepend_map(as_list * list, struct as_map_s * value) 
{
	return as_util_hook(prepend, 1, list, (as_val *) value);
}

/******************************************************************************
 *	ITERATION FUNCTIONS
 *****************************************************************************/

/**
 *	Call the callback function for each element in the list..
 *
 *	@param list		The list to iterate over.
 *	@param callback	The callback function call for each element.
 *	@param udata	User-data to send to the callback.
 *
 *	@return 0 on success. Otherwise an error ocucrred.
 */
inline bool as_list_foreach(const as_list * list, as_list_foreach_callback callback, void * udata) 
{
	return as_util_hook(foreach, false, list, callback, udata);
}

/**
 *	Creates and initializes a new heap allocated iterator over the given list.
 *
 *	@param list 	The list to iterate.
 *
 *	@return On success, a new as_iterator. Otherwise NULL.
 */
inline union as_list_iterator_u * as_list_iterator_new(const as_list * list) 
{
	return as_util_hook(iterator_new, NULL, list);
}


/**
 *	Initializes a stack allocated iterator over the given list.
 *
 *	@param list 	The list to iterate.
 *	@param it 		The iterator to initialize.
 *
 *	@return On success, the initializes as_iterator. Otherwise NULL.
 */
inline union as_list_iterator_u * as_list_iterator_init(union as_list_iterator_u * it, const as_list * list) 
{
	return as_util_hook(iterator_init, NULL, list, it);
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_list_toval(as_list * list) 
{
	return (as_val *) list;
}

/**
 *	Convert from an as_val.
 */
inline as_list * as_list_fromval(as_val * v) 
{
	return as_util_fromval(v, AS_LIST, as_list);
}

/******************************************************************************
 * as_val FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_list_val_destroy(as_val * v);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_list_val_hashcode(const as_val * v);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_list_val_tostring(const as_val * v);
