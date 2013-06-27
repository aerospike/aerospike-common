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

#include <aerospike/as_list.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

/**
 *	An dynamic array implementation for as_list.
 *
 *	To use the list, you can either initialize a stack allocated list, 
 *	using `as_arraylist_init()`:
 *
 *	~~~~~~~~~~{.c}
 *		as_arraylist list;
 *		as_arraylist_init(&list, 2, 0);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated list using `as_arraylist_new()`:
 *
 *	~~~~~~~~~~{.c}
 *		as_arraylist * list = as_arraylist_new(2, 0);
 *	~~~~~~~~~~
 *
 *	When you are finished using the list, then you should release the list and
 *	associated resources, using `as_arraylist_destroy()`:
 *	
 *	~~~~~~~~~~{.c}
 *		as_arraylist_destroy(list);
 *	~~~~~~~~~~
 *
 *	
 *	The `as_arraylist` is a subtype of `as_list`. This allows you to 
 *	alternatively use `as_list` functions, by typecasting `as_arraylist` to
 *	`as_list`.
 *
 *	~~~~~~~~~~{.c}
 *		as_arraylist list;
 *		as_list * l = (as_list *) as_arraylist_init(&list, 3, 0);
 *		as_list_append_int64(l, 1);
 *		as_list_append_int64(l, 2);
 *		as_list_append_int64(l, 3);
 *		as_list_destroy(l);
 *	~~~~~~~~~~
 *	
 *	Each of the `as_list` functions proxy to the `as_arraylist` functions.
 *	So, calling `as_list_destroy()` is equivalent to calling 
 *	`as_arraylist_destroy()`.
 *
 */
typedef struct as_arraylist_s {

	/**
	 *	@private
	 *	as_arraylist is an as_list.
	 *	You can cast as_arraylist to as_list.
	 */
	as_list _;

	/**
	 *	Number of elements to add, when capacity is reached.
	 *	If 0 (zero), then capacity can't be expanded.
	 */
	uint32_t block_size;

	/**
	 *	The total number elements allocated.
	 */
	uint32_t capacity;

	/**
	 *	The number of elements used.
	 */
	uint32_t size;

	/**
	 *	The elements of the list.
	 */
	as_val ** elements;

} as_arraylist;

/**
 *	Status codes for various as_arraylist operations.
 */
typedef enum as_arraylist_status_e {
	
	/**
	 *	Normal operation.
	 */
	AS_ARRAYLIST_OK         = 0,

	/**
	 *	Unable to expand capacity, because realloc() failed.
	 */
	AS_ARRAYLIST_ERR_ALLOC  = 1,

	/**
	 *	Unable to expand capacity, because as_arraylist.block_size is 0.
	 */
	AS_ARRAYLIST_ERR_MAX    = 2

} as_arraylist_status;

/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated list as an as_arraylist.
 *
 *	@param list 		The as_list to initialize
 *	@param capacity		The number of elements to allocate to the list.
 *	@param block_size	The number of elements to grow the list by, when the 
 *	capacity has been reached.
 *
 *	@return On success, the initialize list. Otherwise NULL.
 */
as_arraylist * as_arraylist_init(as_arraylist * list, uint32_t capacity, uint32_t block_size);

/**
 *	Create and initialize a heap allocated list as as_arraylist.
 *
 *	@param list 		The as_list to initialize
 *	@param capacity		The number of elements to allocate to the list.
 *	@param block_size	The number of elements to grow the list by, when the 
 *	capacity has been reached.
 *  
 *	@return On success, the new list. Otherwise NULL.
 */
as_arraylist * as_arraylist_new(uint32_t capacity, uint32_t block_size);

/**
 *	Destoy the list and release resources.
 *
 *	@param list	The list to destroy.
 */
void as_arraylist_destroy(as_arraylist * list);

/*******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

/**
 *  The hash value of the list.
 *
 *	@param list 	The list.
 *
 *	@return The hash value of the list.
 */
uint32_t as_arraylist_hashcode(const as_arraylist * list);

/**
 *  The number of elements in the list.
 *
 *	@param list 	The list.
 *
 *	@return The number of elements in the list.
 */
uint32_t as_arraylist_size(const as_arraylist * list);

/*******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 ******************************************************************************/

/**
 *  Add the element to the end of the list.
 *
 *	@param list 	The list.
 *	@param value 	The value to prepend.
 *
 *	@return AS_ARRAYLIST_OK on success. Otherwise an error occurred.
 */
int as_arraylist_append(as_arraylist * list, as_val * value);

/**
 *  Add the element to the beginning of the list.
 *
 *	@param list 	The list.
 *	@param value 	The value to prepend.
 *
 *	@return AS_ARRAYLIST_OK on success. Otherwise an error occurred.
 */
int as_arraylist_prepend(as_arraylist * list, as_val * value);

/**
 *  Return the element at the specified index.
 *
 *	@param list 	The list.
 *	@param index	The index of the element.
 *
 *	@return The value at given index, if it exists. Otherwise NULL.
 */
as_val * as_arraylist_get(const as_arraylist * list, const uint32_t index);

/**
 *  Set the value for for the given index of the list.
 *
 *  Notice that in order to maintain proper object/memory management, we
 *  just first destroy (as_val_destroy()) the old object at element position(i)
 *  before assigning the new element.  Also note that the object at element
 *  position (i) is assumed to exist, so all element positions must be
 *  appropriately initialized to zero.
 *
 *	@param list 	The list.
 *	@param index	Position in the list.
 *	@param value	The value to set at the given index.
 *
 *	@return AS_ARRAYLIST_OK on success. Otherwise an error occurred.
 */
int as_arraylist_set(as_arraylist * list, const uint32_t index, as_val * value);

/**
 *	Get the first element of the list.
 *
 *	@param list 	The list to get the first element from.
 *
 *	@return The first element of the list. Otherwise NULL.
 */
as_val * as_arraylist_head(const as_arraylist * list);

/**
 *  Returns a new list containing all elements other than the head
 *
 *	@param list 	The list to get the elements from.
 *
 *	@return A new list of all elements after the first element.
 */
as_arraylist * as_arraylist_tail(const as_arraylist * list);

/**
 *  Return a new list with the first n elements removed.
 *
 *	@param list 	The list.
 *	@param n 		The number of elements to remove.
 *
 *	@return A new list of all elements after the first n elements.
 */
as_arraylist * as_arraylist_drop(const as_arraylist * list, uint32_t n);

/**
 *  Return a new list containing the first n elements.
 *
 *	@param list 	The list.
 *
 *	@return A new list of the first n elements.
 */
as_arraylist * as_arraylist_take(const as_arraylist * list, uint32_t n);

/******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

/** 
 *  Call the callback function for each element in the list.
 *
 *	@param list 	The list to iterate.
 *	@param callback	The functon to call for each element in the list.
 *	@param udata	User-data to be sent to the callback.
 *
 *	@param TRUE on success. Otherwise FALSE.
 */
bool as_arraylist_foreach(const as_arraylist * list, as_list_foreach_callback callback, void * udata);

/******************************************************************************
 *	MACROS
 ******************************************************************************/

#define as_arraylist_get_int64(__list, __i) 		as_list_get_int64(((as_list *) (__list)), __i)
#define as_arraylist_get_str(__list, __i) 			as_list_get_str(((as_list *) (__list)), __i)
#define as_arraylist_get_integer(__list, __i) 		as_list_get_integer(((as_list *) (__list)), __i)
#define as_arraylist_get_string(__list, __i) 		as_list_get_string(((as_list *) (__list)), __i)
#define as_arraylist_get_bytes(__list, __i) 		as_list_get_bytes(((as_list *) (__list)), __i)
#define as_arraylist_get_list(__list, __i) 			as_list_get_list(((as_list *) (__list)), __i)
#define as_arraylist_get_map(__list, __i) 			as_list_get_map(((as_list *) (__list)), __i)

#define as_arraylist_set_int64(__list, __i, __v) 	as_list_set_int64(((as_list *) (__list)), __i, __v)
#define as_arraylist_set_str(__list, __i, __v) 		as_list_set_str(((as_list *) (__list)), __i, __v)
#define as_arraylist_set_integer(__list, __i, __v) 	as_list_set_integer(((as_list *) (__list)), __i, __v)
#define as_arraylist_set_string(__list, __i, __v) 	as_list_set_string(((as_list *) (__list)), __i, __v)
#define as_arraylist_set_bytes(__list, __i, __v) 	as_list_set_bytes(((as_list *) (__list)), __i, __v)
#define as_arraylist_set_list(__list, __i, __v) 	as_list_set_list(((as_list *) (__list)), __i, __v)
#define as_arraylist_set_map(__list, __i, __v) 		as_list_set_map(((as_list *) (__list)), __i, __v)

#define as_arraylist_append_int64(__list, __v) 		as_list_append_int64(((as_list *) (__list)), __v)
#define as_arraylist_append_str(__list, __v) 		as_list_append_str(((as_list *) (__list)), __v)
#define as_arraylist_append_integer(__list, __v) 	as_list_append_integer(((as_list *) (__list)), __v)
#define as_arraylist_append_string(__list, __v) 	as_list_append_string(((as_list *) (__list)), __v)
#define as_arraylist_append_bytes(__list, __v) 		as_list_append_bytes(((as_list *) (__list)), __v)
#define as_arraylist_append_list(__list, __v) 		as_list_append_list(((as_list *) (__list)), __v)
#define as_arraylist_append_map(__list, __v) 		as_list_append_map(((as_list *) (__list)), __v)

#define as_arraylist_prepend_int64(__list, __v) 	as_list_prepend_int64(((as_list *) (__list)), __v)
#define as_arraylist_prepend_str(__list, __v) 		as_list_prepend_str(((as_list *) (__list)), __v)
#define as_arraylist_prepend_integer(__list, __v) 	as_list_prepend_integer(((as_list *) (__list)), __v)
#define as_arraylist_prepend_string(__list, __v) 	as_list_prepend_string(((as_list *) (__list)), __v)
#define as_arraylist_prepend_bytes(__list, __v) 	as_list_prepend_bytes(((as_list *) (__list)), __v)
#define as_arraylist_prepend_list(__list, __v) 		as_list_prepend_list(((as_list *) (__list)), __v)
#define as_arraylist_prepend_map(__list, __v) 		as_list_prepend_map(((as_list *) (__list)), __v)

#define as_arraylist_inita(__list, __n)\
	as_arraylist_init(__list, 0);\
	(__list)->capacity = __n;\
	(__list)->size = 0;\
	(__list)->elements = alloca(sizeof(as_val *) * __n);


