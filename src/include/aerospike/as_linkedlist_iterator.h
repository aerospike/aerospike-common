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
#include <aerospike/as_linkedlist.h>

#include <stdbool.h>
 
/******************************************************************************
 *	TYPES
 *****************************************************************************/

/**
 *	Iterator for as_linkedlist
 *
 *	To use the iterator, you can either intialize a stack allocated variable,
 *	using `as_linkedlist_iterator_init()`:
 *
 *	~~~~~~~~~~{.c}
 *		as_linkedlist_iterator it;
 *		as_linkedlist_iterator_init(&it, &list);
 *	~~~~~~~~~~
 * 
 *	Or you can create a new heap allocated variable using 
 *	`as_linkedlist_iterator_new()`:
 *
 *	~~~~~~~~~~{.c}
 *		as_linkedlist_iterator * it = as_linkedlist_iterator_new(&list);
 *	~~~~~~~~~~
 *
 *	To iterate, use `as_linkedlist_iterator_has_next()` and 
 *	`as_linkedlist_iterator_next()`:
 *
 *	~~~~~~~~~~{.c}
 *		while ( as_linkedlist_iterator_has_next(&it) ) {
 *			const as_val * val = as_linkedlist_iterator_next(&it);
 *		}
 *	~~~~~~~~~~
 *
 *	When you are finished using the iterator, then you should release the 
 *	iterator and associated resources:
 *	
 *	~~~~~~~~~~{.c}
 *		as_linkedlist_iterator_destroy(it);
 *	~~~~~~~~~~
 *	
 *
 *	The `as_linkedlist_iterator` is a subtype of  `as_iterator`. This allows you
 *	to alternatively use `as_iterator` functions, by typecasting 
 *	`as_linkedlist_iterator` to `as_iterator`.
 *
 *	~~~~~~~~~~{.c}
 *		as_linkedlist_iterator it;
 *		as_iterator * i = (as_iterator *) as_linkedlist_iterator_init(&it, &list);
 *
 *		while ( as_iterator_has_next(i) ) {
 *			const as_val * as_iterator_next(i);
 *		}
 *
 *		as_iterator_destroy(i);
 *	~~~~~~~~~~
 *
 *	Each of the `as_iterator` functions proxy to the `as_linkedlist_iterator`
 *	functions. So, calling `as_iterator_destroy()` is equivalent to calling
 *	`as_linkedlist_iterator_destroy()`.
 *
 */
typedef struct as_linkedlist_iterator_s {

	/**
	 *	as_linkedlist_iterator is an as_iterator.
	 *	You can cast as_linkedlist_iterator to as_iterator.
	 */
	as_iterator _;

	/**
	 *	The current list
	 *	Iterating moves to the next tail.
	 */
	const as_linkedlist * list;

} as_linkedlist_iterator;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

/**
 *	Initializes a stack allocated as_iterator for the given as_linkedlist.
 *
 *	@param iterator 	The iterator to initialize.
 *	@param list 		The list to iterate.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 */
as_linkedlist_iterator * as_linkedlist_iterator_init(as_linkedlist_iterator * iterator, const as_linkedlist * list);

/**
 *	Creates a new heap allocated as_iterator for the given as_linkedlist.
 *
 *	@param list 		The list to iterate.
 *
 *	@return On success, the new iterator. Otherwise NULL.
 */
as_linkedlist_iterator * as_linkedlist_iterator_new(const as_linkedlist * list);

/**
 *	Destroy the iterator and releases resources used by the iterator.
 *
 *	@param iterator 	The iterator to release
 */
void as_linkedlist_iterator_destroy(as_linkedlist_iterator * iterator);

/******************************************************************************
 *	ITERATOR FUNCTIONS
 *****************************************************************************/

/**
 *	Tests if there are more values available in the iterator.
 *
 *	@param iterator 	The iterator to be tested.
 *
 *	@return true if there are more values. Otherwise false.
 */
bool as_linkedlist_iterator_has_next(const as_linkedlist_iterator * i);

/**
 *	Attempts to get the next value from the iterator.
 *	This will return the next value, and iterate past the value.
 *
 *	@param iterator 	The iterator to get the next value from.
 *
 *	@return The next value in the list if available. Otherwise NULL.
 */
const as_val * as_linkedlist_iterator_next(as_linkedlist_iterator * i);
