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

#include <citrusleaf/alloc.h>

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_iterator.h>

#include <stdbool.h>
#include <stdlib.h>

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_iterator_hooks as_arraylist_iterator_hooks;

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

as_arraylist_iterator * as_arraylist_iterator_init(as_arraylist_iterator * iterator, const as_arraylist * list)
{
	if ( !iterator ) return iterator;

	as_iterator_init((as_iterator *) iterator, false, NULL, &as_arraylist_iterator_hooks);
	iterator->list = list;
	iterator->pos = 0;
	return iterator;
}

as_arraylist_iterator * as_arraylist_iterator_new(const as_arraylist * list)
{
	as_arraylist_iterator * iterator = (as_arraylist_iterator *) cf_malloc(sizeof(as_arraylist_iterator));
	if ( !iterator ) return iterator;

	as_iterator_init((as_iterator *) iterator, true, NULL, &as_arraylist_iterator_hooks);
	iterator->list = list;
	iterator->pos = 0;
	return iterator;
}

bool as_arraylist_iterator_release(as_arraylist_iterator * iterator) 
{
	iterator->list = NULL;
	iterator->pos = 0;
	return true;
}

void as_arraylist_iterator_destroy(as_arraylist_iterator * iterator) 
{
	as_iterator_destroy((as_iterator *) iterator);
}

bool as_arraylist_iterator_has_next(const as_arraylist_iterator * iterator) 
{
	return iterator && iterator->pos < iterator->list->size;
}

const as_val * as_arraylist_iterator_next(as_arraylist_iterator * iterator) 
{
	if ( iterator->pos < iterator->list->size ) {
		as_val * val = *(iterator->list->elements + iterator->pos);
		iterator->pos++;
		return val;
	}
	return NULL;
}
