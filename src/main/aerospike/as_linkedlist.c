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
#include <aerospike/as_list.h>

#include <stdlib.h>
#include <stdio.h>

#include "internal.h"

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_list_hooks as_linkedlist_list_hooks;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

as_linkedlist * as_linkedlist_init(as_linkedlist * list, as_val * head, as_linkedlist * tail)
{
	if ( !list ) return list;

	as_list_init((as_list *) list, false, NULL, &as_linkedlist_list_hooks);
	list->head = head;
	list->tail = tail;
	return list;
}

as_linkedlist * as_linkedlist_new(as_val * head, as_linkedlist * tail) 
{
	as_linkedlist * list = (as_linkedlist *) malloc(sizeof(as_linkedlist));
	if ( !list ) return list;

	as_list_init((as_list *) list, true, NULL, &as_linkedlist_list_hooks);
	list->head = head;
	list->tail = tail;
	return list;
}

bool as_linkedlist_release(as_linkedlist * list)
{
	if ( list->head ) as_val_destroy(list->head);
	if ( list->tail ) as_linkedlist_destroy(list->tail);
	return true;
}

void as_linkedlist_destroy(as_linkedlist * list)
{
	as_list_destroy((as_list *) list);
}

/*******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

uint32_t as_linkedlist_hashcode(const as_linkedlist * list) 
{
	return 0;
}

uint32_t as_linkedlist_size(const as_linkedlist * list) 
{
	return !list ? 0 : 
		(list->head ? 1 : 0) + 
		(list->tail ? as_linkedlist_size(list->tail) : 0);
}

/*******************************************************************************
 *	ACCESSOR & MODIFICATION FUNCTIONS
 ******************************************************************************/

/**
 *	Recurse through the link to find the last entry
 */
as_linkedlist * as_linkedlist_last(as_linkedlist * list) 
{
	if ( !list ) return list;
	if ( list->tail == 0 ) return list;
	return as_linkedlist_last(list->tail);
}


int as_linkedlist_append(as_linkedlist * list, as_val * value) 
{
	as_linkedlist * last = as_linkedlist_last(list);
	
	if ( !last ) return 2;

	if ( last->tail ) return 3;

	if ( !last->head ) {
		last->head = value;
	}
	else {
		last->tail = as_linkedlist_new(value, NULL);
	}

	return 0;
}

int as_linkedlist_prepend(as_linkedlist * list, as_val * value) 
{
	as_linkedlist * tail  = as_linkedlist_new(list->head, list->tail);
	list->head = value;
	list->tail = tail;
	return 0;
}

as_val * as_linkedlist_get(const as_linkedlist * list, const uint32_t i) 
{
	for (int j = 0; j < i && list != NULL; j++) {
		if (list->tail == 0) {
			return NULL;
		}
		list = list->tail;
	}
	return list->head;
}

int as_linkedlist_set(as_linkedlist * list, const uint32_t i, as_val * value) 
{
	for (int j = 0; j < i ; j++) {
		if (list->tail == 0) {
			return(1);
		}
		list = list->tail;
	}

	as_val_destroy(list->head);
	list->head = value;

	return 0;
}

as_val * as_linkedlist_head(const as_linkedlist * list) 
{
	as_val_reserve(list->head);
	return list->head;
}

/**
 *	Return the all elements except the head element.
 */
as_linkedlist * as_linkedlist_tail(const as_linkedlist * list) 
{
	as_linkedlist * tail  = list->tail;
	as_val_reserve(tail);
	return tail;
}

/**
 *	Create a list by taking all elements after the first n elements.
 *	The elements are ref-counted, so, the new list will share a reference
 *	with the original list.
 */
as_linkedlist * as_linkedlist_drop(const as_linkedlist * list, uint32_t n) 
{
	as_linkedlist * first = NULL;
	as_linkedlist * last = NULL;

	for (int i = 0; i < n; i++ ) {
		if ( !list->tail ) return 0;
		list = list->tail;
	}

	while (list) {
		as_val_reserve(list->head);
		if ( !first ) {
			first = last = as_linkedlist_new(list->head, NULL);
		} 
		else {
			last->tail = as_linkedlist_new(list->head, NULL);
			last = last->tail;
		}

		if ( !list->tail ) {
			break;
		}

		list = list->tail;
	}
	
	return first;
}

/**
 *	Create a list by taking the first n elements.
 *	The elements are ref-counted, so, the new list will share a reference
 *	with the original list.
 */
as_linkedlist * as_linkedlist_take(const as_linkedlist * list, uint32_t n) 
{
	as_linkedlist * first = NULL;
	as_linkedlist * last = NULL;
	
	int i = 1;
	while (list) {
		as_val_reserve(list->head);

		if ( !first ) {
			first = last = as_linkedlist_new(list->head, 0);;
		} 
		else {
			last->tail = as_linkedlist_new(list->head,NULL);
			last = last->tail;
		}

		list = list->tail;

		if ( i++ >= n ) {
			break;
		}
	}

	return first;
}

/******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

/** 
 *	Call the callback function for each element in the list.
 */
bool as_linkedlist_foreach(const as_linkedlist * list, as_list_foreach_callback callback, void * udata) 
{
	while ( list != NULL && list->head != NULL ) {
		if ( callback(list->head, udata) == false ) {
			return false;
		}
		list = list->tail;
	}
	return true;
}
