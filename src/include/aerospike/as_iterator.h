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

#include <stdbool.h>

/******************************************************************************
 *	TYPES
 ******************************************************************************/

struct as_iterator_hooks_s;

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
	 *	Data for the iterator.
	 */
	void * data;

	/**
	 *	Hooks for subtypes of as_iterator.
	 */
	const struct as_iterator_hooks_s * hooks;

} as_iterator;

/**
 *	Iterator Function Hooks
 */
typedef struct as_iterator_hooks_s {

	/**
	 *	Releases the subtype of as_iterator.
	 */
	bool (* destroy)(as_iterator *);

	/**
	 *	Tests whether there is another element in the iterator.
	 */
	bool (* has_next)(const as_iterator *);

	/**
	 *	Read the next value.
	 */
	const as_val * (* next)(as_iterator *);

} as_iterator_hooks;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated iterator.
 */
as_iterator * as_iterator_init(as_iterator * iterator, bool free, void * data, const as_iterator_hooks * hooks);

/**
 *	Destroys the iterator and releasing associated resources.
 */
void as_iterator_destroy(as_iterator * iterator);

/******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

/**
 *	Tests if there are more values available in the iterator.
 *
 *	@param iterator		The iterator to be tested.
 *
 *	@return true if there are more values, otherwise false.
 */
inline bool as_iterator_has_next(const as_iterator * iterator)
{
	return as_util_hook(has_next, false, iterator);
}

/**
 *	Attempts to get the next value from the iterator.
 *	This will return the next value, and iterate past the value.
 *
 *	@param iterator		The iterator to get the next value from.
 *
 *	@return the next value available in the iterator.
 */
inline const as_val * as_iterator_next(as_iterator * iterator)
{
	return as_util_hook(next, NULL, iterator);
}
