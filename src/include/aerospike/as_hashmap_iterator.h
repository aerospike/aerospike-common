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

#include <aerospike/as_hashmap.h>
#include <aerospike/as_iterator.h>

#include <stdbool.h>

/******************************************************************************
 *	TYPES
 ******************************************************************************/

/**
 *	Iterator for as_hashmap.
 *
 *	To use the iterator, you can either intialize a stack allocated variable,
 *	use `as_hashmap_iterator_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_hashmap_iterator it;
 *	as_hashmap_iterator_init(&it, &map);
 *	~~~~~~~~~~
 * 
 *	Or you can create a new heap allocated variable using 
 *	`as_hashmap_iterator_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_hashmap_iterator * it = as_hashmap_iterator_new(&map);
 *	~~~~~~~~~~
 *	
 *	To iterate, use `as_hashmap_iterator_has_next()` and 
 *	`as_hashmap_iterator_next()`:
 *
 *	~~~~~~~~~~{.c}
 *	while ( as_hashmap_iterator_has_next(&it) ) {
 *		const as_val * val = as_hashmap_iterator_next(&it);
 *	}
 *	~~~~~~~~~~
 *
 *	When you are finished using the iterator, then you should release the 
 *	iterator and associated resources:
 *	
 *	~~~~~~~~~~{.c}
 *	as_hashmap_iterator_destroy(it);
 *	~~~~~~~~~~
 *	
 *
 *	The `as_hashmap_iterator` is a subtype of  `as_iterator`. This allows you
 *	to alternatively use `as_iterator` functions, by typecasting 
 *	`as_hashmap_iterator` to `as_iterator`.
 *
 *	~~~~~~~~~~{.c}
 *	as_hashmap_iterator it;
 *	as_iterator * i = (as_iterator *) as_hashmap_iterator_init(&it, &map);
 *
 *	while ( as_iterator_has_next(i) ) {
 *		const as_val * as_iterator_next(i);
 *	}
 *
 *	as_iterator_destroy(i);
 *	~~~~~~~~~~
 *	
 *	Each of the `as_iterator` functions proxy to the `as_hashmap_iterator`
 *	functions. So, calling `as_iterator_destroy()` is equivalent to calling
 *	`as_hashmap_iterator_destroy()`.
 *
 *	@extends as_iterator
 */
typedef struct as_hashmap_iterator_s {

	as_iterator _;

	/**
	 *	The hashmap
	 */
	void * htable;

	/**
	 *	Current entry
	 */
	void * curr;

	/**
	 *	Next entry
	 */
	void * next;

	/**
	 *	Position
	 */
	uint32_t pos;

	/**
	 *	Number of entries
	 */
	uint32_t size;

} as_hashmap_iterator;

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

/**
 *	Initializes a stack allocated as_iterator for the given as_hashmap.
 *
 *	@param iterator 	The iterator to initialize.
 *	@param map			The map to iterate.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 *
 *	@relatesalso as_hashmap_iterator
 */
as_hashmap_iterator * as_hashmap_iterator_init(as_hashmap_iterator * iterator, const as_hashmap * map);

/**
 *	Creates a heap allocated as_iterator for the given as_hashmap.
 *
 *	@param map 			The map to iterate.
 *
 *	@return On success, the new iterator. Otherwise NULL.
 *
 *	@relatesalso as_hashmap_iterator
 */
as_hashmap_iterator * as_hashmap_iterator_new(const as_hashmap * map);

/**
 *	Destroy the iterator and releases resources used by the iterator.
 *
 *	@param iterator 	The iterator to release
 *
 *	@relatesalso as_hashmap_iterator
 */
void as_hashmap_iterator_destroy(as_hashmap_iterator * iterator);


/******************************************************************************
 *	ITERATOR FUNCTIONS
 *****************************************************************************/

/**
 *	Tests if there are more values available in the iterator.
 *
 *	@param iterator 	The iterator to be tested.
 *
 *	@return true if there are more values. Otherwise false.
 *
 *	@relatesalso as_hashmap_iterator
 */
bool as_hashmap_iterator_has_next(const as_hashmap_iterator * iterator);

/**
 *	Attempts to get the next value from the iterator.
 *	This will return the next value, and iterate past the value.
 *
 *	@param iterator 	The iterator to get the next value from.
 *
 *	@return The next value in the list if available. Otherwise NULL.
 *
 *	@relatesalso as_hashmap_iterator
 */
const as_val * as_hashmap_iterator_next(as_hashmap_iterator * iterator);
