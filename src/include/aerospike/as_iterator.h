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

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <aerospike/as_linkedlist_iterator.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_hashmap_iterator.h>

#include <stdbool.h>

/******************************************************************************
 *	TYPES
 ******************************************************************************/

struct as_iterator_hooks_s;

/**
 *	Iterator Data
 *
 *	Each is specific to an implementation of `as_iterator`.
 */
typedef union as_iterator_data_u {
	as_linkedlist_iterator	linkedlist;
	as_arraylist_iterator	arraylist;
	as_hashmap_iterator		hashmap;
	void *					generic;
} as_iterator_data;

/**
 *	Iterator Object
 */
typedef struct as_iterator_s {
	
	/**
	 *	@private
	 *	If TRUE, then free this instance.
	 */
	bool free;

	/**
	 *	Data provided by the implementation of `as_iterator`.
	 */
	union as_iterator_data_u data;

	/**
	 *	Hooks provided by the implementations of `as_iterator`.
	 */
	const struct as_iterator_hooks_s * hooks;

} as_iterator;

/**
 *	Iterator Function Hooks
 */
typedef struct as_iterator_hooks_s {

	/**
	 *	Releases the implementation of `as_iterator`.
	 *
	 *	@param it	The iterator to destroy.
	 */
	void (* const destroy)(as_iterator * it);

	/**
	 *	Tests whether there is another element in the iterator.
	 *
	 *	@param it	The iterator to test.
	 *
	 *	@return true, if there is a next value. Otherwise false.
	 */
	bool (* const has_next)(const as_iterator * it);

	/**
	 *	Read the next value.
	 *
	 *	@param it	The iterator to read the next value from.
	 *
	 *	@return On success, the next value from the iterator. Otherwise NULL.
	 */
	as_val * (* const next)(as_iterator * it);

} as_iterator_hooks;

/******************************************************************************
 *	FUNCTIONS
 ******************************************************************************/

/**
 *	Initializes a stack allocated `as_iterator`.
 *
 *	@param i		The iterator to initialize.
 *	@param data		The data provided by the implementation of the iterator.
 *	@param hooks	The hooks provided by the implementation of the iterator.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 */
as_iterator * as_iterator_init(as_iterator * i, void * data, const as_iterator_hooks * hooks);

/**
 *	Create and initialize a heap allocated `as_iterator`.
 *
 *	this is done with MALLOC and thus the iterator must be freed exactly once
 *
 *	@param data		The data provided by the implementation of the iterator.
 *	@param hooks	The hooks provided by the implementation of the iterator.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 */
as_iterator * as_iterator_new(void * data, const as_iterator_hooks * hooks);

/**
 *	Destroy the iterator, releasing associated resources.
 *
 *	@param i		The iterator to destroy.
 */
void as_iterator_destroy(as_iterator * i);

/******************************************************************************
 *	INLINE FUNCTIONS
 ******************************************************************************/

/**
 *	Tests if there are more values available in the iterator.
 *
 *	@param i	The  iterator to be tested.
 *
 *	@return On success, the next value available in the iterator. Otherwise NULL.
 */
inline bool as_iterator_has_next(const as_iterator * i) {
	return as_util_hook(has_next, false, i);
}

/**
 *	Attempts to get the next value from the iterator.
 *	This will return the next value, and iterate past the value.
 *
 *	@param i 	The iterator to get the next value from.
 *
 *	@return On success, the next value available in the iterator. Otherwise NULL.
 */
inline const as_val * as_iterator_next(as_iterator * i) {
	return as_util_hook(next, NULL, i);
}
