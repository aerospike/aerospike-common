#include "as_linkedlist.h"
#include <stdlib.h>
#include <stdio.h>
#include <cf_alloc.h>
#include "internal.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static as_linkedlist * as_linkedlist_end(as_linkedlist *);

//
// as_list Implementation
//
static int              as_linkedlist_list_free(as_list *);
static uint32_t         as_linkedlist_list_hash(const as_list *);
static uint32_t         as_linkedlist_list_size(const as_list *);
static int              as_linkedlist_list_append(as_list *, as_val *);
static int              as_linkedlist_list_prepend(as_list *, as_val *);
static as_val *         as_linkedlist_list_get(const as_list *, const uint32_t);
static int              as_linkedlist_list_set(as_list *, const uint32_t, as_val *);
static as_val *         as_linkedlist_list_head(const as_list *);
static as_list *        as_linkedlist_list_tail(const as_list *);
static as_list *        as_linkedlist_list_drop(const as_list *, uint32_t n);
static as_list *        as_linkedlist_list_take(const as_list *, uint32_t n);
static as_iterator *    as_linkedlist_list_iterator(const as_list *);

//
// as_iterator Implementation
//
static const int        as_linkedlist_iterator_free(as_iterator *);
static const bool       as_linkedlist_iterator_has_next(const as_iterator *);
static const as_val *   as_linkedlist_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

//
// as_list Interface
//
const as_list_hooks as_linkedlist_list = {
    .free       = as_linkedlist_list_free,
    .hash       = as_linkedlist_list_hash,
    .size       = as_linkedlist_list_size,
    .append     = as_linkedlist_list_append,
    .prepend    = as_linkedlist_list_prepend,
    .get        = as_linkedlist_list_get,
    .set        = as_linkedlist_list_set,
    .head       = as_linkedlist_list_head,
    .tail       = as_linkedlist_list_tail,
    .drop       = as_linkedlist_list_drop,
    .take       = as_linkedlist_list_take,
    .foreach    = NULL,                     // @TODO: add implementation
    .iterator   = as_linkedlist_list_iterator
};

//
// as_iterator Interface
//
const as_iterator_hooks as_linkedlist_iterator = {
    .free       = as_linkedlist_iterator_free,
    .has_next   = as_linkedlist_iterator_has_next,
    .next       = as_linkedlist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_linkedlist * as_linkedlist_init(as_linkedlist * l, as_val * head, as_linkedlist * tail) {
    if ( !l ) return l;
    l->head = head;
    l->tail = tail;
    return l;
}

int as_linkedlist_destroy(as_linkedlist * l) {
    if ( !l ) return 0;
    if ( l->head ) as_val_free(l->head);
    l->head = NULL;
    if ( l->tail ) {
        as_linkedlist_free(l->tail);
    }
    l->tail = NULL;
    return 0;
}

as_linkedlist * as_linkedlist_new(as_val * head, as_linkedlist * tail) {
    as_linkedlist * l = (as_linkedlist *) cf_rc_alloc(sizeof(as_linkedlist));
    return as_linkedlist_init(l, head, tail);
}

int as_linkedlist_free(as_linkedlist * l) {
    if ( !l ) return 0;
    LOG("as_arraylist_free: release");
    if ( cf_rc_release(l) > 0 ) return 0;
    as_linkedlist_destroy(l);
    cf_rc_free(l);
    LOG("as_arraylist_free: free");
    return 0;
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static uint32_t as_linkedlist_size(as_linkedlist * l) {
    return (l && l->head ? 1 : 0) + (l && l->tail ? as_linkedlist_size(l->tail) : 0);
}

static as_linkedlist * as_linkedlist_end(as_linkedlist * l) {
    return ((l && l->tail) ? as_linkedlist_end(l->tail) : l);
}

static int as_linkedlist_list_free(as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) l->source;
    as_linkedlist_free(ll);
    l->source = NULL;
    return 0;
}

static uint32_t as_linkedlist_list_hash(const as_list * l) {
    return 0;
}

static uint32_t as_linkedlist_list_size(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return as_linkedlist_size(ll);
}

static int as_linkedlist_list_append(as_list * l, as_val * v) {
    as_linkedlist * ll = (as_linkedlist *) as_list_source(l);
    if ( !ll ) return 1;

    as_linkedlist * lle  = as_linkedlist_end(ll);
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
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    as_linkedlist * tl  = as_linkedlist_new(ll->head, ll->tail);
    ll->head = v;
    ll->tail = tl;
    return 0;
}

static as_val * as_linkedlist_list_get(const as_list * l, const uint32_t i) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    for (int j = 0; j < i && ll != NULL; j++) {
        ll = ll->tail;
    }
    return (ll ? ll->head : NULL);
}

static int as_linkedlist_list_set(as_list * l, const uint32_t i, as_val * v) {
    
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    for (int j = 0; j < i && ll != NULL; j++) {
        ll = ll->tail;
    }

    if ( !ll ) return 1;

    as_val_free(ll->head);
    ll->head = v;

    return 0;
}

static as_val * as_linkedlist_list_head(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return ll->head;
}

static as_list * as_linkedlist_list_tail(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    as_linkedlist * tl  = ll->tail;
    cf_rc_reserve(tl);
    return as_list_new(tl, &as_linkedlist_list);
}

static as_list * as_linkedlist_list_drop(const as_list * l, uint32_t n) {

    as_linkedlist * p = (as_linkedlist *) l->source;
    as_linkedlist * h = as_linkedlist_new(NULL,NULL);
    as_linkedlist * t = h;

    for (int i = 0; p && i < n; i++ ) {
        p = p->tail;
    }

    while ( p ) {
        t->head = as_val_ref(p->head);
        t->tail = as_linkedlist_new(NULL,NULL);
        t = t->tail;
        p = p->tail;
    }
    
    return as_list_new(h, &as_linkedlist_list);
}

static as_list * as_linkedlist_list_take(const as_list * l, uint32_t n) {
    
    as_linkedlist * p = (as_linkedlist *) l->source;
    as_linkedlist * h = as_linkedlist_new(NULL,NULL);
    as_linkedlist * t = h;
    
    for (int i = 0; p && i < n; i++ ) {
        t->head = as_val_ref(p->head);
        t->tail = as_linkedlist_new(NULL,NULL);
        t = t->tail;
        p = p->tail;
    }

    return as_list_new(h, &as_linkedlist_list);
}

static as_iterator * as_linkedlist_list_iterator(const as_list * l) {
    if ( l == NULL ) return NULL;
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) malloc(sizeof(as_linkedlist_iterator_source));
    source->list = (as_linkedlist *) l->source;
    return as_iterator_new(source, &as_linkedlist_iterator);
}



static const bool as_linkedlist_iterator_has_next(const as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
    return source && source->list && source->list->head;
}

static const as_val * as_linkedlist_iterator_next(as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
    as_val * head = NULL;
    if ( source && source->list ) {
        head = source->list->head;
        source->list = source->list->tail;
    }
    return head;
}

static const int as_linkedlist_iterator_free(as_iterator * i) {
    if ( !i ) return 0;
    if ( i->source ) free(i->source);
    i->source = NULL;
    i->hooks = NULL;
    return 0;
}
