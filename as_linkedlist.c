#include "as_linkedlist.h"
#include <stdlib.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_linkedlist_s as_linkedlist;
typedef struct as_linkedlist_iterator_source_s as_linkedlist_iterator_source;

struct as_linkedlist_s {
    as_val *    head;
    as_list *   tail;
};


struct as_linkedlist_iterator_source_s {
    const as_list * list;
};

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_linkedlist_free(as_list *);
static uint32_t as_linkedlist_hash(const as_list *);
static uint32_t as_linkedlist_size(const as_list *);
static int as_linkedlist_append(as_list *, as_val *);
static int as_linkedlist_prepend(as_list *, as_val *);
static as_val * as_linkedlist_get(const as_list *, const uint32_t);
static int as_linkedlist_set(as_list *, const uint32_t, as_val *);
static as_val * as_linkedlist_head(const as_list *);
static as_list * as_linkedlist_tail(const as_list *);
static as_iterator * as_linkedlist_iterator(const as_list *);

static as_list * as_linkedlist_last(as_list *);

static const int as_linkedlist_iterator_free(as_iterator *);
static const bool as_linkedlist_iterator_has_next(const as_iterator *);
static const as_val * as_linkedlist_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_list_hooks as_linkedlist_hooks = {
    .free       = as_linkedlist_free,
    .hash       = as_linkedlist_hash,
    .size       = as_linkedlist_size,
    .append     = as_linkedlist_append,
    .prepend    = as_linkedlist_prepend,
    .get        = as_linkedlist_get,
    .set        = as_linkedlist_set,
    .head       = as_linkedlist_head,
    .tail       = as_linkedlist_tail,
    .iterator   = as_linkedlist_iterator
};

static const as_iterator_hooks as_linkedlist_iterator_hooks = {
    .free       = as_linkedlist_iterator_free,
    .has_next   = as_linkedlist_iterator_has_next,
    .next       = as_linkedlist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list * as_linkedlist_new(as_val * head, as_list * tail) {
    as_linkedlist * l = (as_linkedlist *) malloc(sizeof(as_linkedlist));
    l->head = head;
    l->tail = tail;
    return as_list_new(l, &as_linkedlist_hooks);
}

static int as_linkedlist_free(as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    if ( ll != NULL ) {
        if ( ll->head ) as_val_free(ll->head);
        if ( ll->tail ) as_list_free(ll->tail);
        free(l);
    }
    return 0;
}

static uint32_t as_linkedlist_hash(const as_list * l) {
    return 0;
}

static uint32_t as_linkedlist_size(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return (ll->head ? 1 : 0) + (ll->tail ? as_list_size(ll->tail) : 0);
}

static int as_linkedlist_append(as_list * l, as_val * v) {
    as_list * tl  = as_linkedlist_last(l);
    if ( !tl ) return 1;

    as_linkedlist * tll = (as_linkedlist *) as_list_source(tl);
    if ( !tll ) return 2;
    
    if ( tll->tail ) return 3;

    if ( !tll->head ) {
        tll->head = (as_val *) v;
    }
    else {
        tll->tail = as_linkedlist_new(v, NULL);
    }
    
    return 0;
}

static int as_linkedlist_prepend(as_list * l, as_val * v) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    as_list *       nl  = as_linkedlist_new(ll->head, ll->tail);
    ll->head = v;
    ll->tail = nl;
    return 0;
}

static as_val * as_linkedlist_get(const as_list * l, const uint32_t i) {
    as_list * t = (as_list *) l;
    for (int j = 0; j < i && t != NULL; j++) {
        t = as_list_tail(t);
    }
    return (t ? as_list_head(t) : NULL);
}

static int as_linkedlist_set(as_list * l, const uint32_t i, as_val * v) {
    as_list * t = (as_list *) l;
    for (int j = 0; j < i && t != NULL; j++) {
        t = as_list_tail(t);
    }
    if ( !t ) return 1;

    as_linkedlist * ll = (as_linkedlist *) as_list_source(t);
    ll->head = v;

    return 0;
}

static as_val * as_linkedlist_head(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return ll->head;
}

static as_list * as_linkedlist_tail(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return ll->tail;
}

static as_list * as_linkedlist_last(as_list * l) {
    as_list * tail  = as_list_tail(l);
    return (tail ? as_linkedlist_last(tail) : l);
}

static as_iterator * as_linkedlist_iterator(const as_list * l) {
    if ( l == NULL ) return NULL;
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) malloc(sizeof(as_linkedlist_iterator_source));
    source->list = l;
    return as_iterator_new(source, &as_linkedlist_iterator_hooks);
}

static const bool as_linkedlist_iterator_has_next(const as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
    return source->list != NULL && as_linkedlist_head(source->list) != NULL;
}

static const as_val * as_linkedlist_iterator_next(as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
    if ( source->list == NULL ) return NULL;
    as_val * head = as_linkedlist_head(source->list);
    source->list = as_linkedlist_tail(source->list);
    return head;
}

static const int as_linkedlist_iterator_free(as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
    if ( source ) free(source);
    free(i);
    return 0;
}
