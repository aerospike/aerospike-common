/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <citrusleaf/alloc.h>

#include <aerospike/as_iterator.h>
#include <aerospike/as_linkedlist.h>
#include <aerospike/as_linkedlist_iterator.h>
#include <aerospike/as_list.h>

#include <stdbool.h>
#include <stdlib.h>

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void     as_linkedlist_iterator_destroy(as_iterator *);
static bool     as_linkedlist_iterator_has_next(const as_iterator *);
static as_val * as_linkedlist_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 *****************************************************************************/

const as_iterator_hooks as_linkedlist_iterator_hooks = {
	.destroy    = as_linkedlist_iterator_destroy,
	.has_next   = as_linkedlist_iterator_has_next,
	.next       = as_linkedlist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_iterator * as_linkedlist_iterator_new(const as_linkedlist * l)
{
	as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
	if ( !i ) return i;

	i->free = true;
	i->hooks = &as_linkedlist_iterator_hooks;
	i->data.linkedlist.list = l;
	return i;
}

as_iterator * as_linkedlist_iterator_init(const as_linkedlist * l, as_iterator * i)
{
	if ( !i ) return i;
	
	i->free = false;
	i->hooks = &as_linkedlist_iterator_hooks;
	i->data.linkedlist.list = l;
	return i;
}

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void as_linkedlist_iterator_destroy(as_iterator * i)
{
	return;
}

static bool as_linkedlist_iterator_has_next(const as_iterator * i)
{
	as_linkedlist_iterator * it = (as_linkedlist_iterator *) &(i->data.linkedlist);
	return it->list && it->list->head;
}

static as_val * as_linkedlist_iterator_next(as_iterator * i)
{
	as_linkedlist_iterator * it = (as_linkedlist_iterator *) &(i->data.linkedlist);
	as_val * head = NULL;
	if ( it->list ) {
		head = it->list->head;
		if ( it->list->tail ) {
			it->list = &(it->list->tail->data.linkedlist);
		}
		else {
			it->list = 0;
		}
	}
	return head;
}
