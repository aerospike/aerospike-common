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

#include <stdlib.h>
#include <stdio.h>

#include <citrusleaf/alloc.h>

#include <aerospike/as_linkedlist.h>
#include <aerospike/as_linkedlist_iterator.h>
#include <aerospike/as_list.h>

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static as_list *        as_linkedlist_end(as_list *);

static bool             as_linkedlist_list_destroy(as_list *);
static uint32_t         as_linkedlist_list_hash(const as_list *);

static uint32_t         as_linkedlist_list_size(const as_list *);
static int              as_linkedlist_list_append(as_list *, as_val *);
static int              as_linkedlist_list_prepend(as_list *, as_val *);
static as_val *         as_linkedlist_list_get(const as_list *, const uint32_t);
static int              as_linkedlist_list_set(as_list *, const uint32_t, as_val *);
static as_val *         as_linkedlist_list_head(const as_list *);
static as_list *        as_linkedlist_list_tail(const as_list *);
static as_list *        as_linkedlist_list_drop(const as_list *, uint32_t);
static as_list *        as_linkedlist_list_take(const as_list *, uint32_t);

static bool             as_linkedlist_list_foreach(const as_list *, as_list_foreach_callback, void *);
static as_iterator *    as_linkedlist_list_iterator_init(const as_list *, as_iterator *);
static as_iterator *    as_linkedlist_list_iterator_new(const as_list *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

const as_list_hooks as_linkedlist_list_hooks = {
    .destroy        = as_linkedlist_list_destroy,
    .hashcode       = as_linkedlist_list_hash,

    .size           = as_linkedlist_list_size,
    .append         = as_linkedlist_list_append,
    .prepend        = as_linkedlist_list_prepend,
    .get            = as_linkedlist_list_get,
    .set            = as_linkedlist_list_set,
    .head           = as_linkedlist_list_head,
    .tail           = as_linkedlist_list_tail,
    .drop           = as_linkedlist_list_drop,
    .take           = as_linkedlist_list_take,
    
    .foreach        = as_linkedlist_list_foreach,
    .iterator_init  = as_linkedlist_list_iterator_init,
    .iterator_new   = as_linkedlist_list_iterator_new
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list * as_linkedlist_init(as_list * l, as_val * head, as_list * tail) {
    as_val_init(&l->_, AS_LIST, false);
    l->hooks = &as_linkedlist_list_hooks;
    l->data.linkedlist.head = head;
    l->data.linkedlist.tail = tail;
    return l;
}

as_list * as_linkedlist_new(as_val * head, as_list * tail) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    if (!l) return l;
    as_val_init(&l->_, AS_LIST, true);
    l->hooks = &as_linkedlist_list_hooks;
    l->data.linkedlist.head = head;
    l->data.linkedlist.tail = tail;
    return l;
}

void as_linkedlist_destroy(as_list *l) {
    as_val_val_destroy( (as_val *) l);
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

/**
 * recurse through the link to find the end
 */
static as_list * as_linkedlist_end(as_list * l) {
    as_linkedlist * ll = &l->data.linkedlist;
    if (ll->tail == 0)  return l;
    return as_linkedlist_end(ll->tail);
}


static bool as_linkedlist_list_destroy(as_list * l) {
    as_linkedlist * ll = &l->data.linkedlist;
    if ( ll->head ) as_val_val_destroy(ll->head);
    if ( ll->tail ) as_val_val_destroy((as_val *)ll->tail);
    return true;
}

static uint32_t as_linkedlist_list_hash(const as_list * l) {
    return 0;
}

static uint32_t as_linkedlist_list_size(const as_list * l) {
    const as_linkedlist * ll = &l->data.linkedlist;
    return (ll->head ? 1 : 0) + (ll->tail ? as_linkedlist_list_size(ll->tail) : 0);
}

static int as_linkedlist_list_append(as_list * l, as_val * v) {

    as_list *       le  = as_linkedlist_end(l);
    as_linkedlist * lle = &le->data.linkedlist;
    
    if ( !lle ) return 2;

    if ( lle->tail ) return 3;

    if ( !lle->head ) {
        lle->head = v;
    }
    else {
        lle->tail = as_linkedlist_new(v, NULL);
    }

    return 0;
}

static int as_linkedlist_list_prepend(as_list * l, as_val * v) {
    as_linkedlist * ll = &l->data.linkedlist;
    as_list * tl  = as_linkedlist_new(ll->head, ll->tail);
    ll->head = v;
    ll->tail = tl;

    return 0;
}

static as_val * as_linkedlist_list_get(const as_list * l, const uint32_t i) {
    const as_linkedlist * ll = &l->data.linkedlist;
    for (int j = 0; j < i && ll != NULL; j++) {
        if (ll->tail == 0) return(0); // ith not available
        ll = &(ll->tail->data.linkedlist);
    }
    return (ll->head);
}

static int as_linkedlist_list_set(as_list * l, const uint32_t i, as_val * v) {
    
    as_linkedlist * ll = &l->data.linkedlist;
    for (int j = 0; j < i ; j++) {
        if (ll->tail == 0) {
            return(1);
        }
        ll = &(ll->tail->data.linkedlist);
    }

    as_val_destroy(ll->head);
    ll->head = v;

    return 0;
}

static as_val * as_linkedlist_list_head(const as_list * l) {
    const as_linkedlist * ll = &l->data.linkedlist;
    as_val_reserve(ll->head);
    return ll->head;
}

/**
 * Return the all elements except the head element.
 */
static as_list * as_linkedlist_list_tail(const as_list * l) {
    const as_linkedlist * ll = &l->data.linkedlist;
    as_list * tl  = ll->tail;
    as_val_reserve(tl);
    return(tl);
}

/**
 * Create a list by taking all elements after the first n elements.
 * The elements are ref-counted, so, the new list will share a reference
 * with the original list.
 */
static as_list * as_linkedlist_list_drop(const as_list * l, uint32_t n) {
    const as_linkedlist * ll = &l->data.linkedlist;

    as_list * h = 0;
    as_list * t = 0;

    for (int i = 0; i < n; i++ ) {
        if (0 == ll->tail) return(0);
        ll = &(ll->tail->data.linkedlist);
    }

    while (ll) {
        as_val_reserve(ll->head);
        if (h == 0) {
            h = t = as_linkedlist_new(ll->head, 0);;
        } else {
            t->data.linkedlist.tail = as_linkedlist_new(ll->head,NULL);
            t = t->data.linkedlist.tail;
        }

        if (0 == ll->tail) break;
        ll = &(ll->tail->data.linkedlist);
    }
    
    return h;
}

/**
 * Create a list by taking the first n elements.
 * The elements are ref-counted, so, the new list will share a reference
 * with the original list.
 */
static as_list * as_linkedlist_list_take(const as_list * l, uint32_t n) {
    
    const as_linkedlist * ll = &l->data.linkedlist;

    as_list * h = 0;
    as_list * t = 0;
    
    int i = 1;
    while (ll) {
        as_val_reserve(ll->head);
        if (h == 0) {
            h = t = as_linkedlist_new(ll->head, 0);;
        } else {
            t->data.linkedlist.tail = as_linkedlist_new(ll->head,NULL);
            t = t->data.linkedlist.tail;
        }
        ll = &(ll->tail->data.linkedlist);
        if (i++ >= n) break;
    }

    return h;
}


/** 
 * Call the callback function for each element in the list.
 */
static bool as_linkedlist_list_foreach(const as_list * l, bool (*foreach)(as_val * val, void * udata), void * udata) {
    const as_linkedlist * ll = (as_linkedlist *) &l->data.linkedlist;
    while ( ll != NULL && ll->head != NULL ) {
        if ( foreach(ll->head, udata) == false ) {
            return false;
        }
        ll = &(ll->tail->data.linkedlist);
    }
    return true;
}

/**
 * Initializes an iterator for this list
 */
static as_iterator * as_linkedlist_list_iterator_init(const as_list * l, as_iterator *i) {
    return as_linkedlist_iterator_init(&l->data.linkedlist, i);
}

/**
 * Creates a new iterator for this list
 */
static as_iterator * as_linkedlist_list_iterator_new(const as_list * l) {
    return as_linkedlist_iterator_new(&l->data.linkedlist);
}
