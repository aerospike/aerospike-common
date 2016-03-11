/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

/**
 *  double linked list functionality
 */

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <citrusleaf/cf_ll.h>
#include <citrusleaf/alloc.h>

/**
 * SYNOPSIS
 * LinkedList
 * Sometimes the answer is a doubly linked list. It's not that frequent, but
 * all the corner cases in a double linked list can be annoying.
 */
 
/******************************************************************************
 * MACROS
 ******************************************************************************/

#define LL_UNLOCK(_ll) 	if ( _ll->uselock ) { pthread_mutex_unlock(&(_ll->LOCK)); }
#define LL_LOCK(_ll)	if ( _ll->uselock ) { pthread_mutex_lock(&(_ll->LOCK)); }

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void cf_ll_prepend_lockfree(cf_ll * ll, cf_ll_element * e) {
	// empty list
	if (ll->head == 0) { 
		ll->head = e;
		ll->tail = e;
		e->next = 0;
		e->prev = 0;
	}

	// at least one element - add to head
	else {
		e->next = ll->head;
		e->prev = 0;
		
		ll->head->prev = e;
		ll->head = e;
	}
	
	ll->sz++;
}

void cf_ll_prepend(cf_ll *ll, cf_ll_element *e  ) {
	LL_LOCK(ll);
	cf_ll_prepend_lockfree(ll, e);
	LL_UNLOCK(ll)
}

void cf_ll_append_lockfree(cf_ll *ll, cf_ll_element *e  ) {
	if (ll->head == 0) { 
		ll->head = e;
		ll->tail = e;
		e->next = 0;
		e->prev = 0;
	}
	// at least one element - add to tail
	else {
		e->next = 0;
		e->prev = ll->tail;
		
		ll->tail->next = e;
		ll->tail = e;
	}
	
	ll->sz++;
}


void cf_ll_append(cf_ll *ll, cf_ll_element *e  ) {
	LL_LOCK(ll);

	cf_ll_append_lockfree(ll, e);

	LL_UNLOCK(ll);
}

void cf_ll_insert_after_lockfree(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {
	ins->next = cur->next;
	ins->prev = cur;
	
	if (cur->next == 0)
		ll->tail = ins;
	else
		cur->next->prev = ins;

	cur->next = ins;
	
	ll->sz++;
}


void cf_ll_insert_after(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {
	LL_LOCK(ll);
	cf_ll_insert_after_lockfree(ll, cur, ins);
	LL_UNLOCK(ll);
}

void cf_ll_insert_before_lockfree(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {

	ins->next = cur;
	ins->prev = cur->prev;

	if (cur->prev == 0)
		ll->head = ins;
	else
		cur->prev->next = ins;
	cur->prev = ins;
	
	ll->sz++;
}

void cf_ll_insert_before(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {
	LL_LOCK(ll);
	cf_ll_insert_before_lockfree( ll, cur, ins);
	LL_UNLOCK(ll);
}


void cf_ll_delete_lockfree(cf_ll *ll, cf_ll_element *e ) {
	// make empty
	if (ll->sz == 1) {
		ll->head = 0;
		ll->tail = 0;
	}
	
	// head delete
	else if (e == ll->head) {
		ll->head = e->next;
		e->next->prev = 0;
	}
	// tail delete
	else if (e == ll->tail) {
		ll->tail = e->prev;
		e->prev->next = 0;
	}
	// we're in the middle somewhere
	else {
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
	
	ll->sz --;
	
	if (ll->destroy_fn)
		(ll->destroy_fn) (e);
}

void cf_ll_delete(cf_ll *ll, cf_ll_element *e ) {
	// extra check for fun
	if (ll->sz == 0)	return;

	LL_LOCK(ll);

	cf_ll_delete_lockfree(ll, e);
	
	LL_UNLOCK(ll);
	
}


int cf_ll_reduce( cf_ll *ll, bool forward, cf_ll_reduce_fn fn, void *udata) {
	int rv = 0;
	
	LL_LOCK(ll);
	
	cf_ll_element *cur = forward ? ll->head : ll->tail;
	
	while (cur) {
		
		if ( (rv = (*fn) (cur, udata) ) ) {
			
			if (rv == CF_LL_REDUCE_DELETE) {

				cf_ll_element *next = forward ? cur->next : cur->prev;
				// Calls the destructor on cur, can't touch it after that
				cf_ll_delete_lockfree(ll, cur);
				cur = next;
				
			}
			else
				goto Exit;
		}
		else {
			cur = forward ? cur->next : cur->prev;
		}
	}

Exit:		
	LL_UNLOCK(ll);
	return(rv);
}

int cf_ll_insert_reduce(cf_ll *ll, cf_ll_element *e, bool forward, cf_ll_reduce_fn fn, void *udata) {
	int rv = 0;
	LL_LOCK(ll);

	cf_ll_element *cur = forward ? ll->head : ll->tail;
	
	while (cur) {
		
		if ( (rv = (*fn) (cur, udata) ) ) {
			
			if (rv == CF_LL_REDUCE_INSERT) {

				if (forward)
					cf_ll_insert_before_lockfree(ll, cur, e);
				else
					cf_ll_insert_after_lockfree(ll, cur, e);
				rv = 0;
				
			}
			
			goto Exit;
			
		}
		else {
			cur = forward ? cur->next : cur->prev;
		}
	}
	
	// give chance to insert at "end"
	if ( (rv = (*fn) (cur, udata) ) ) {
		if (rv == CF_LL_REDUCE_INSERT) {
			if (forward)
				cf_ll_append_lockfree(ll, e);
			else
				cf_ll_prepend_lockfree(ll, e);
			rv = 0;
		}
	}

Exit:
	LL_UNLOCK(ll);
	return(rv);
}

uint32_t cf_ll_size(cf_ll *ll) {
	LL_LOCK(ll);
	uint32_t sz = ll->sz;
	LL_UNLOCK(ll);
	return(sz);
}	

cf_ll_element * cf_ll_search_lockfree(cf_ll *ll, cf_ll_element *e, bool forward, cf_ll_reduce_fn fn) {
	if (ll->head == NULL) {
		return NULL;
	}
	int rv = 0;
	cf_ll_element *cur = forward ? ll->head : ll->tail;
	
	while (cur) {
		rv = (*fn) (cur, (void*)e);

		if (rv == CF_LL_REDUCE_MATCHED) {
			return cur;
		}
		else if (rv == CF_LL_REDUCE_NOT_MATCHED) {
			cur = forward ? cur->next : cur->prev;
			rv = 0;
		}
		else {
			return NULL;
		}
	}
	return NULL;
}

cf_ll_element * cf_ll_search(cf_ll *ll, cf_ll_element *e, bool forward, cf_ll_reduce_fn fn) 
{
	LL_LOCK(ll);
	
	cf_ll_element * ele = NULL;
	ele = cf_ll_search_lockfree(ll, e, forward, fn);

	LL_UNLOCK(ll);

	return ele;
}

cf_ll_iterator * cf_ll_getIterator(cf_ll * ll, bool forward)
{
	cf_ll_iterator *iter = NULL;
	iter = (cf_ll_iterator *)cf_malloc(sizeof(cf_ll_iterator));
	if (iter == NULL) {
		return NULL;
	}
	iter->forward = forward;
	iter->next = iter->forward ? ll->head : ll->tail;
	return iter;
}

void cf_ll_releaseIterator(cf_ll_iterator *iter)
{
	if (iter) cf_free(iter);
}

cf_ll_element * cf_ll_getNext(cf_ll_iterator *iter)
{
	if (!iter) return NULL;
	cf_ll_element * ele = iter->next;
	if (ele != NULL) {
		iter->next = iter->forward ? ele->next : ele->prev;
	}
	return ele;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimante
 * and so on. If the index is out of range NULL is returned. */
cf_ll_element *cf_ll_index(cf_ll *ll, int index)
{
	cf_ll_element *ele = NULL;
	if (index < 0) {
		index = (-index) - 1;
		ele = ll->tail;
		while (index-- && ele) {
			ele = ele->prev;
		}
	}
	else {
		ele = ll->head;
		while (index-- && ele) {
			ele = ele->next;
		}
	}
	return ele;
}

int  cf_ll_init(cf_ll *ll, cf_ll_destructor destroy_fn, bool uselock) {
	ll->head = 0;
	ll->tail = 0;
	ll->destroy_fn = destroy_fn;
	ll->sz = 0;
	ll->uselock = uselock;
	if (uselock) {
		pthread_mutex_init(&ll->LOCK, 0);
	}
	return(0);	
}
