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

#include <aerospike/as_linkedlist.h>
#include <aerospike/as_linkedlist_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_list.h>
#include <aerospike/as_list_iterator.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "internal.h"


/******************************************************************************
 *	EXTERN FUNCTIONS
 ******************************************************************************/

extern bool as_linkedlist_release(as_linkedlist * list);

/******************************************************************************
 *	STATIC FUNCTIONS
 ******************************************************************************/

static bool _as_linkedlist_list_destroy(as_list * l) 
{
	return as_linkedlist_release((as_linkedlist *) l);
}

static uint32_t _as_linkedlist_list_hashcode(const as_list * l) 
{
	return as_linkedlist_hashcode((as_linkedlist *) l);
}

static uint32_t _as_linkedlist_list_size(const as_list * l) 
{
	return as_linkedlist_size((as_linkedlist *) l);
}

static int _as_linkedlist_list_append(as_list * l, as_val * v) 
{
	return as_linkedlist_append((as_linkedlist *) l, v);
}

static int _as_linkedlist_list_prepend(as_list * l, as_val * v) 
{
	return as_linkedlist_prepend((as_linkedlist *) l, v);
}

static as_val * _as_linkedlist_list_get(const as_list * l, const uint32_t i) 
{
	return as_linkedlist_get((as_linkedlist *) l, i);
}

static int _as_linkedlist_list_set(as_list * l, const uint32_t i, as_val * v) 
{
	return as_linkedlist_set((as_linkedlist *) l, i, v);
}

static as_val * _as_linkedlist_list_head(const as_list * l) 
{
	return as_linkedlist_head((as_linkedlist *) l);
}

static as_list * _as_linkedlist_list_tail(const as_list * l) 
{
	return (as_list *) as_linkedlist_tail((as_linkedlist *) l);
}

static as_list * _as_linkedlist_list_drop(const as_list * l, uint32_t n) 
{
	return (as_list *) as_linkedlist_drop((as_linkedlist *) l, n);
}

static as_list * _as_linkedlist_list_take(const as_list * l, uint32_t n) 
{
	return (as_list *) as_linkedlist_take((as_linkedlist *) l, n);
}

static bool _as_linkedlist_list_foreach(const as_list * l, as_list_foreach_callback callback, void * udata) 
{
	return as_linkedlist_foreach((as_linkedlist *) l, callback, udata);
}

static as_list_iterator * _as_linkedlist_list_iterator_new(const as_list * l) 
{
	return (as_list_iterator *) as_linkedlist_iterator_new((as_linkedlist *) l);
}

static as_list_iterator * _as_linkedlist_list_iterator_init(const as_list * l, as_list_iterator * it) 
{
	return (as_list_iterator *) as_linkedlist_iterator_init((as_linkedlist_iterator *) it, (as_linkedlist *) l);
}

/******************************************************************************
 *	HOOKS
 ******************************************************************************/

const as_list_hooks as_linkedlist_list_hooks = {

	/***************************************************************************
	 *	instance hooks
	 **************************************************************************/

	.destroy	= _as_linkedlist_list_destroy,

	/***************************************************************************
	 *	info hooks
	 **************************************************************************/

	.hashcode	= _as_linkedlist_list_hashcode,
	.size		= _as_linkedlist_list_size,

	/***************************************************************************
	 *	accessor and modifier hooks
	 **************************************************************************/

	.append		= _as_linkedlist_list_append,
	.prepend	= _as_linkedlist_list_prepend,
	.get		= _as_linkedlist_list_get,
	.set		= _as_linkedlist_list_set,
	.head		= _as_linkedlist_list_head,
	.tail		= _as_linkedlist_list_tail,
	.drop		= _as_linkedlist_list_drop,
	.take		= _as_linkedlist_list_take,

	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/

	.foreach		= _as_linkedlist_list_foreach,
	.iterator_new	= _as_linkedlist_list_iterator_new,
	.iterator_init	= _as_linkedlist_list_iterator_init

};
