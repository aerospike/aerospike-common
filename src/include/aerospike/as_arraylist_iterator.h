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

#include <aerospike/as_arraylist.h>

/*******************************************************************************
 *	TYPES
 ******************************************************************************/

struct as_iterator_s;

/**
 *	Iterator for `as_arraylist`.
 *
 *	To use the iterator, you can either intialize a stack allocated variable,
 *	using `as_arraylist_iterator_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator it;
 *	as_arraylist_iterator_init(&list, &it);
 *	~~~~~~~~~~
 * 
 *	Or you can create a new heap allocated variable, using 
 *	`as_arraylist_iterator_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator * it = as_arraylist_iterator_new(&list);
 *	~~~~~~~~~~
 *
 *	To iterate, use `as_iterator_has_next()` and `as_iterator_next()`:
 *
 *	~~~~~~~~~~{.c}
 *	while ( as_iterator_has_next(&it) ) {
 *		const as_val * val = as_iterator_next(&it);
 *	}
 *	~~~~~~~~~~
 *
 *	When you are finished using the iterator, then you should release the 
 *	iterator and associated resources:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator_destroy(it);
 *	~~~~~~~~~~
 *	
 *	@see as_iterator
 *	@implements as_iterator
 */
typedef struct as_arraylist_iterator_s {

	/**
	 *	The as_arraylist being iterated over
	 */
	const as_arraylist * list;

	/**
	 *	The current position of the iteration
	 */
	uint32_t pos;

} as_arraylist_iterator;

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

/**
 *	Initialize a stack allocated `as_iterator` for the given `as_arraylist`.
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator it;
 *	as_arraylist_iterator_init(&list, &it);
 *	~~~~~~~~~~
 *
 *	When you are finished with the iterator, you should release the resources
 *	via `as_iterator_destroy()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator_destroy(&it);
 *	~~~~~~~~~~
 *
 *	@param list 		The list to iterate.
 *	@param iterator 	The iterator to initialize.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 */
struct as_iterator_s * as_arraylist_iterator_init(const as_arraylist * list, struct as_iterator_s * iterator);

/**
 *	Creates a new heap allocated `as_iterator` for the given `as_arraylist`.
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator * it = as_arraylist_iterator_new(&list);
 *	~~~~~~~~~~
 *
 *	When you are finished with the iterator, you should release the resources
 *	via `as_iterator_destroy()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator_destroy(&it);
 *	~~~~~~~~~~
 *
 *	@param list 		The list to iterate.
 *
 *	@return On success, the new iterator. Otherwise NULL.
 */
struct as_iterator_s * as_arraylist_iterator_new(const as_arraylist * list);

