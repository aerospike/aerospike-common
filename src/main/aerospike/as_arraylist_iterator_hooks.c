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

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_iterator.h>

#include <stdbool.h>
#include <stdlib.h>

/******************************************************************************
 *	EXTERN FUNCTIONS
 *****************************************************************************/

extern bool as_arraylist_iterator_release(as_arraylist_iterator * iterator);

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

static bool _as_arraylist_iterator_destroy(as_iterator * i) 
{
	return as_arraylist_iterator_release((as_arraylist_iterator *) i);
}

static bool _as_arraylist_iterator_has_next(const as_iterator * i) 
{
	return as_arraylist_iterator_has_next((const as_arraylist_iterator *) i);
}

static const as_val * _as_arraylist_iterator_next(as_iterator * i) 
{
	return as_arraylist_iterator_next((as_arraylist_iterator *) i);
}

/******************************************************************************
 *	HOOKS
 *****************************************************************************/

const as_iterator_hooks as_arraylist_iterator_hooks = {
	.destroy    = _as_arraylist_iterator_destroy,
	.has_next   = _as_arraylist_iterator_has_next,
	.next       = _as_arraylist_iterator_next
};
