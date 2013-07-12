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

/******************************************************************************
 *	TYPES
 ******************************************************************************/

struct as_iterator_s;

/**
 *	Iterator for `as_hashmap`.
 *
 *	To use the iterator, you can either intialize a stack allocated variable,
 *	using `as_arraylist_iterator_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator it;
 *	as_hashmap_iterator_init(&map, &it);
 *	~~~~~~~~~~
 * 
 *	Or you can create a new heap allocated variable, using 
 *	`as_hashmap_iterator_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator * it = as_hashmap_iterator_new(&map);
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
typedef struct as_hashmap_iterator_s {

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
 *	Initializes a stack allocated `as_iterator` for the given `as_hashmap`.
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator it;
 *	as_hashmap_iterator_init(&map, &it);
 *	~~~~~~~~~~
 *
 *	When you are finished with the iterator, you should release the resources
 *	via `as_iterator_destroy()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_iterator_destroy(&it);
 *	~~~~~~~~~~
 *
 *	@param map			The map to iterate.
 *	@param iterator 	The iterator to initialize.
 *
 *	@return On success, the initialized iterator. Otherwise NULL.
 */
struct as_iterator_s * as_hashmap_iterator_init(const as_hashmap * map, struct as_iterator_s * iterator);

/**
 *	Creates a heap allocated `as_iterator` for the given `as_hashmap`.
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
 *	@param map			The map to iterate.
 *
 *	@return On success, the new iterator. Otherwise NULL.
 */
struct as_iterator_s * as_hashmap_iterator_new(const as_hashmap * map);
