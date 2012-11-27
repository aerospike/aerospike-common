#include "as_linkedlist.h"
#include <stdlib.h>

typedef struct as_linkedlist_s as_linkedlist;
typedef struct as_linkedlist_iterator_source_s as_linkedlist_iterator_source;

static const as_list_hooks as_linkedlist_hooks;
static const as_iterator_hooks as_linkedlist_iterator_hooks;

static int as_linkedlist_free(as_list *);
static uint32_t as_linkedlist_size(const as_list *);
static int as_linkedlist_append(as_list *, as_val *);
static int as_linkedlist_prepend(as_list *, as_val *);
static as_val * as_linkedlist_get(const as_list *, uint32_t);
static as_val * as_linkedlist_head(const as_list *);
static as_list * as_linkedlist_tail(const as_list *);
static as_list * as_linkedlist_take(const as_list *, uint32_t);
static as_list * as_linkedlist_drop(const as_list *, uint32_t);
static as_iterator * as_linkedlist_iterator(const as_list *);



struct as_linkedlist_s {
    as_val *    head;
    as_list *   tail;
};

struct as_linkedlist_iterator_source_s {
    const as_list * list;
};


as_list * cons(as_val * h, as_list * t) {
    return as_linkedlist_new(h,t);
}




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

static uint32_t as_linkedlist_size(const as_list * l) {
    // as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return 0;
}

static int as_linkedlist_append(as_list * l, as_val * v) {
    // as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return 0;
}

static int as_linkedlist_prepend(as_list * l, as_val * v) {
    // as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return 0;
}

static as_val * as_linkedlist_get(const as_list * l, uint32_t i) {
    // as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return NULL;
}

static as_val * as_linkedlist_head(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return ll->head;
}

static as_list * as_linkedlist_tail(const as_list * l) {
    as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return ll->tail;
}

static as_list * as_linkedlist_take(const as_list * l, uint32_t n) {
    // as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return (as_list *) l;
}

static as_list * as_linkedlist_drop(const as_list * l, uint32_t n) {
    // as_linkedlist * ll  = (as_linkedlist *) as_list_source(l);
    return (as_list *) l;
}

static as_iterator * as_linkedlist_iterator(const as_list * l) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) malloc(sizeof(as_linkedlist_iterator_source));
    source->list = l;
    return as_iterator_new(source, &as_linkedlist_iterator_hooks);
}

static const bool as_linkedlist_iterator_has_next(const as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
    return source->list != NULL;
}

static const as_val * as_linkedlist_iterator_next(as_iterator * i) {
    as_linkedlist_iterator_source * source = (as_linkedlist_iterator_source *) as_iterator_source(i);
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

static const as_iterator_hooks as_linkedlist_iterator_hooks = {
    .has_next   = as_linkedlist_iterator_has_next,
    .next       = as_linkedlist_iterator_next,
    .free       = as_linkedlist_iterator_free
};

static const as_list_hooks as_linkedlist_hooks = {
    .free       = as_linkedlist_free,
    .size       = as_linkedlist_size,
    .append     = as_linkedlist_append,
    .prepend    = as_linkedlist_prepend,
    .get        = as_linkedlist_get,
    .head       = as_linkedlist_head,
    .tail       = as_linkedlist_tail,
    .take       = as_linkedlist_take,
    .drop       = as_linkedlist_drop,
    .iterator   = as_linkedlist_iterator
};