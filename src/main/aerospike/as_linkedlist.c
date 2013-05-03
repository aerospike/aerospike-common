#include <stdlib.h>
#include <stdio.h>
#include <cf_alloc.h>

#include "as_linkedlist.h"

#include "internal.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static as_list * as_linkedlist_end(as_list *);

//
// as_list Implementation
//
static void             as_linkedlist_list_destroy(as_list *);
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

static bool             as_linkedlist_list_foreach(const as_list * l, void * udata, bool (*foreach)(as_val * val, void * udata));

static as_iterator *    as_linkedlist_list_iterator_init(const as_list *, as_iterator *);
static as_iterator *    as_linkedlist_list_iterator_new(const as_list *);
//
// as_iterator Implementation
//
static void       as_linkedlist_iterator_destroy(as_iterator *);
static bool       as_linkedlist_iterator_has_next(const as_iterator *);
static as_val *   as_linkedlist_iterator_next(as_iterator *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

//
// as_list Interface
//
const as_list_hooks as_linkedlist_list_hooks = {
    .destroy        = as_linkedlist_list_destroy,
    .hash           = as_linkedlist_list_hash,
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

//
// as_iterator Interface
//
const as_iterator_hooks as_linkedlist_iterator_hooks = {
    .destroy       = as_linkedlist_iterator_destroy,
    .has_next   = as_linkedlist_iterator_has_next,
    .next       = as_linkedlist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list * as_linkedlist_init(as_list * l, as_val * head, as_list * tail) {
    as_val_init(&l->_, AS_LIST, false /*is_malloc*/);
    l->hooks = &as_linkedlist_list_hooks;
    l->u.linkedlist.head = head;
    l->u.linkedlist.tail = tail;
    return l;
}

as_list * as_linkedlist_new(as_val * head, as_list * tail) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    as_val_init(&l->_, AS_LIST, true /*is_malloc*/);
    l->hooks = &as_linkedlist_list_hooks;
    l->u.linkedlist.head = head;
    l->u.linkedlist.tail = tail;
    return l;
}

// external, call this
void as_linkedlist_destroy(as_list *l) {
    as_val_val_destroy( (as_val *) l);
}

void as_linkedlist_list_destroy(as_list * l) {
    as_linkedlist_source *ll = &l->u.linkedlist;
    if ( ll->head ) as_val_val_destroy(ll->head);
    if ( ll->tail ) as_val_val_destroy((as_val *)ll->tail);
}

#if 1
void linkedlist_dump(const char *msg, const as_list *l) {
    int i=0;
    fprintf(stderr, "%s: linkedlist dump:\n",msg);
    while (l) {
        const as_linkedlist_source *s = & (l->u.linkedlist);
        char *c = as_val_tostring(s->head);
        fprintf(stderr, "linkedlist: e %d list %p val %s\n",i,l,c);
        free(c);
        i++;
        l = s->tail;
    }
}
#endif

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

//static uint32_t as_linkedlist_size(as_list * l) {
//    as_linkedlist_source *ll = &l->u.linkedlist;
//    return (ll->head ? 1 : 0) + (ll->tail ? as_linkedlist_size(ll->tail) : 0);
//}

//
// recurse through the link to find the end
// likely to pop the stack and crash
static as_list * as_linkedlist_end(as_list * l) {
    as_linkedlist_source *ll = &l->u.linkedlist;
    if (ll->tail == 0)  return(l);
    return as_linkedlist_end(ll->tail);
}



static uint32_t as_linkedlist_list_hash(const as_list * l) {
    return 0;
}

static uint32_t as_linkedlist_list_size(const as_list * l) {
    const as_linkedlist_source *ll = &l->u.linkedlist;
    return (ll->head ? 1 : 0) + (ll->tail ? as_linkedlist_list_size(ll->tail) : 0);
}

static int as_linkedlist_list_append(as_list * l, as_val * v) {

    as_list * le  = as_linkedlist_end(l);
    as_linkedlist_source *lle = &le->u.linkedlist;
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
    as_linkedlist_source *ll = &l->u.linkedlist;
    as_list * tl  = as_linkedlist_new(ll->head, ll->tail);
    ll->head = v;
    ll->tail = tl;

    return 0;
}

static as_val * as_linkedlist_list_get(const as_list * l, const uint32_t i) {
    const as_linkedlist_source *ll = &l->u.linkedlist;
    for (int j = 0; j < i && ll != NULL; j++) {
        if (ll->tail == 0) return(0); // ith not available
        ll = &(ll->tail->u.linkedlist);
    }
    return (ll->head);
}

static int as_linkedlist_list_set(as_list * l, const uint32_t i, as_val * v) {
    
    as_linkedlist_source *ll = &l->u.linkedlist;
    for (int j = 0; j < i ; j++) {
        if (ll->tail == 0) {
            return(1);
        }
        ll = &(ll->tail->u.linkedlist);
    }

    as_val_destroy(ll->head);
    ll->head = v;

    return 0;
}

static as_val * as_linkedlist_list_head(const as_list * l) {
    const as_linkedlist_source *ll = &l->u.linkedlist;
    as_val_reserve(ll->head);
    return ll->head;
}

// this is not the _tail_ this is the non-head component
static as_list * as_linkedlist_list_tail(const as_list * l) {
    const as_linkedlist_source *ll = &l->u.linkedlist;
    as_list * tl  = ll->tail;
    cf_rc_reserve(tl);
    return(tl);
}

// create a duplicate list from element n forward
// the values are _shared_ with the parent
static as_list * as_linkedlist_list_drop(const as_list * l, uint32_t n) {
    const as_linkedlist_source *ll = &l->u.linkedlist;

    as_list * h = 0;
    as_list * t = 0;

    for (int i = 0; i < n; i++ ) {
        if (0 == ll->tail) return(0);
        ll = &(ll->tail->u.linkedlist);
    }

    while (ll) {
        as_val_reserve(ll->head);
        if (h == 0) {
            h = t = as_linkedlist_new(ll->head, 0);;
        } else {
            t->u.linkedlist.tail = as_linkedlist_new(ll->head,NULL);
            t = t->u.linkedlist.tail;
        }

        if (0 == ll->tail) break;
        ll = &(ll->tail->u.linkedlist);
    }
    
    return h;
}

static as_list * as_linkedlist_list_take(const as_list * l, uint32_t n) {
    
    const as_linkedlist_source *ll = &l->u.linkedlist;

    as_list * h = 0;
    as_list * t = 0;

//    linkedlist_dump("TAKE start", l);
    
    int i = 1;
    while (ll) {
        as_val_reserve(ll->head);
        if (h == 0) {
            h = t = as_linkedlist_new(ll->head, 0);;
        } else {
            t->u.linkedlist.tail = as_linkedlist_new(ll->head,NULL);
            t = t->u.linkedlist.tail;
        }
        ll = &(ll->tail->u.linkedlist);
        if (i++ >= n) break;
    }

//    linkedlist_dump("TAKE end", h);
    return(h);
}


static bool as_linkedlist_list_foreach(const as_list * l, void * udata, bool (*foreach)(as_val * val, void * udata)) {
    const as_linkedlist_source * ll = (as_linkedlist_source *)&l->u.arraylist;
    while ( ll != NULL && ll->head != NULL ) {
        if ( foreach(ll->head, udata) == false ) {
            return false;
        }
        ll = &(ll->tail->u.linkedlist);
    }
    return true;
}

static as_iterator * as_linkedlist_list_iterator_init(const as_list * l, as_iterator *i) {
    i->is_malloc = false;
    i->hooks = &as_linkedlist_iterator_hooks;
    const as_linkedlist_source *ll = &l->u.linkedlist;
    i->u.linkedlist.list = ll;
    return i;
}


static as_iterator * as_linkedlist_list_iterator_new(const as_list * l) {
    as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
    i->is_malloc = true;
    i->hooks = &as_linkedlist_iterator_hooks;
    const as_linkedlist_source *ll = &l->u.linkedlist;
    i->u.linkedlist.list = ll;
    return i;
}

static bool as_linkedlist_iterator_has_next(const as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) &(i->u.linkedlist);
    return source->list && source->list->head;
}

static as_val * as_linkedlist_iterator_next(as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) &(i->u.linkedlist);
    as_val * head = NULL;
    if ( source->list ) {
        head = source->list->head;
        if ( source->list->tail) {
            source->list = &(source->list->tail->u.linkedlist);
        }
        else {
            source->list = 0;
        }
    }
    return head;
}

static void as_linkedlist_iterator_destroy(as_iterator * i) {
    return;
}
