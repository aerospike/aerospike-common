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

/*****************************************************************************
 *	TYPES
 *****************************************************************************/

struct as_linkedlist_s;

/**
 *	A linked-list implementation of `as_list`.
 *
 *	The head contains the value, the tail points to the remainder of the list.
 *	
 *
 *	To use the list, you can either initialize a stack allocated list, 
 *	using `as_linkedlist_init()`:
 *
 *	~~~~~~~~~~{.c}
 *		as_linkedlist list;
 *		as_linkedlist_init(&list, NULL, NULL);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated list using `as_linkedlist_new()`:
 *
 *	~~~~~~~~~~{.c}
 *		as_linkedlist * list = as_linkedlist_new(2, 0);
 *	~~~~~~~~~~
 *
 *	When you are finished using the list, then you should release the list and
 *	associated resources, using `as_linkedlist_destroy()`:
 *	
 *	~~~~~~~~~~{.c}
 *		as_linkedlist_destroy(list);
 *	~~~~~~~~~~
 *
 *
 *	The `as_linkedlist` is a subtype of `as_list`. This allows you to 
 *	alternatively use `as_list` functions, by typecasting `as_linkedlist` to
 *	`as_list`.
 *
 *	~~~~~~~~~~{.c}
 *		as_linkedlist list;
 *		as_list * l = (as_list *) as_linkedlist_init(&list, NULL, NULL);
 *		as_list_append_int64(l, 1);
 *		as_list_append_int64(l, 2);
 *		as_list_append_int64(l, 3);
 *		as_list_destroy(l);
 *	~~~~~~~~~~
 *	
 *	Each of the `as_list` functions proxy to the `as_linkedlist` functions.
 *	So, calling `as_list_destroy()` is equivalent to calling 
 *	`as_linkedlist_destroy()`.
 *
 */
typedef struct as_linkedlist_s {
	
	/**
	 *	@private
	 *	as_arraylist is an as_list.
	 *	You can cast as_linkedlist to as_list.
	 */
	as_list _;

	/**
	 *	The first value
	 */
	as_val * head;
	
	/**
	 *	The remaining list
	 */
	struct as_linkedlist_s * tail;

} as_linkedlist;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a list as a linkedlist.
 *
 *	@param list 	The list to initialize.
 *	@param head		The head value for the list.
 *	@param tail		The tail of the list.
 *
 *	@return On success, the initialized map. Otherwise NULL.
 */
as_linkedlist * as_linkedlist_init(as_linkedlist * list, as_val * head, as_linkedlist * tail);

/**
 *	Create a new list as a linkedlist.
 *
 *	@param head		The head value for the list.
 *	@param tail		The tail of the list.
 *
 *	@return On success, the new map. Otherwise NULL.
 */
as_linkedlist * as_linkedlist_new(as_val * head, as_linkedlist * tail);

/**
 *	Destroy the list and release associated resources.
 *
 *	@param list 	The list to destroy.
 */
void as_linkedlist_destroy(as_linkedlist * list);

/******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

/**
 *  The hash value of the list.
 *
 *	@param list 	The list.
 *
 *	@return The hash value of the list.
 */
uint32_t as_linkedlist_hashcode(const as_linkedlist * list);

/**
 *  The number of elements in the list.
 *
 *	@param list 	The list.
 *
 *	@return The number of elements in the list.
 */
uint32_t as_linkedlist_size(const as_linkedlist * list);

/******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 ******************************************************************************/

/**
 *	Get the last node of the linked list.
 *
 *	@param list	The list.
 *
 *	@return The last node of the linked list.
 */
as_linkedlist * as_linkedlist_last(as_linkedlist * list);

/**
 *  Add the element to the end of the list.
 *
 *	@param list 	The list.
 *	@param value 	The value to prepend.
 *
 *	@return AS_ARRAYLIST_OK on success. Otherwise an error occurred.
 */
int as_linkedlist_append(as_linkedlist * list, as_val * value);

/**
 *  Add the element to the beginning of the list.
 *
 *	@param list 	The list.
 *	@param value 	The value to prepend.
 *
 *	@return AS_ARRAYLIST_OK on success. Otherwise an error occurred.
 */
int as_linkedlist_prepend(as_linkedlist * list, as_val * value);

/**
 *  Return the element at the specified index.
 *
 *	@param list 	The list.
 *	@param index	The index of the element.
 *
 *	@return The value at given index, if it exists. Otherwise NULL.
 */
as_val * as_linkedlist_get(const as_linkedlist * list, const uint32_t i);

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
int as_linkedlist_set(as_linkedlist * list, const uint32_t i, as_val * value);

/**
 *	Get the first element of the list.
 *
 *	@param list 	The list to get the first element from.
 *
 *	@return The first element of the list. Otherwise NULL.
 */
as_val * as_linkedlist_head(const as_linkedlist * list);

/**
 *  Returns a new list containing all elements other than the head
 *
 *	@param list 	The list to get the elements from.
 *
 *	@return A new list of all elements after the first element.
 */
as_linkedlist * as_linkedlist_tail(const as_linkedlist * list);

/**
 *  Return a new list with the first n elements removed.
 *
 *	The elements are ref-counted, so, the new list will share a reference
 *	with the original list.
 *
 *	@param list 	The list.
 *	@param n 		The number of elements to remove.
 *
 *	@return A new list of all elements after the first n elements.
 */
as_linkedlist * as_linkedlist_drop(const as_linkedlist * list, uint32_t n);

/**
 *  Return a new list containing the first n elements.
 *
 *	The elements are ref-counted, so, the new list will share a reference
 *	with the original list.
 *
 *	@param list 	The list.
 *
 *	@return A new list of the first n elements.
 */
as_linkedlist * as_linkedlist_take(const as_linkedlist * list, uint32_t n);

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
bool as_linkedlist_foreach(const as_linkedlist * list, as_list_foreach_callback callback, void * udata);
