#include "as_list.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct as_list_iterator_source_s as_list_iterator_source;
static const as_iterator_hooks as_list_iterator_hooks;


/**
 * List Structure
 * Contains pointer to the head andval  v tail.
 *
 * @field head first element.
 * @field tail as_list containing remaining elements.
 */
struct as_list_s {
    as_val _;
    as_val * head;
    as_list * tail;
};

/**
 * Source for stream iterators
 */
struct as_list_iterator_source_s {
    const as_list * list;
};

static const as_val AS_LIST_VAL;
static int as_list_freeval(as_val *);

static const bool as_list_iterator_has_next(const as_iterator *); 
static const as_val * as_list_iterator_next(as_iterator *);
static const int as_list_iterator_free(as_iterator *);
static const as_iterator_hooks as_list_iterator_hooks;


as_list * cons(as_val * head, as_list * tail) {
    as_list * l = as_list_new();
    l->head = head;
    l->tail = tail;
    return l;
}

as_list * as_list_new() {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    l->_ = AS_LIST_VAL;
    return l;
}

int as_list_free(as_list * l) {
    if ( l != NULL ) {
        if ( l->head ) as_val_free(l->head);
        if ( l->tail ) as_list_free(l->tail);
        free(l);
    }
    return 0;
}

as_val * as_list_toval(const as_list * l) {
    return (as_val *) l;
}

as_list * as_list_fromval(const as_val * v) {
    return as_val_type(v) == AS_LIST ? (as_list *) v : NULL;
}

int as_list_append(as_list * l, as_val * v) {
    return 1;
}

as_val * as_list_get(const int i) {
    return NULL;
}

as_val * as_list_head(const as_list * l) {
    return l->head;
}

as_list * as_list_tail(const as_list * l) {
    return l->tail;
}

static int as_list_freeval(as_val * v) {
    return as_val_type(v) == AS_LIST ? as_list_free((as_list *) v) : 1;
}

as_iterator * as_list_iterator(const as_list * l) {
    as_list_iterator_source * source = (as_list_iterator_source *) malloc(sizeof(as_list_iterator_source));
    source->list = l;
    return as_iterator_new(source, &as_list_iterator_hooks);
}

static const bool as_list_iterator_has_next(const as_iterator * i) {
    as_list_iterator_source * source = (as_list_iterator_source *) as_iterator_source(i);
    return source->list != NULL;
}

static const as_val * as_list_iterator_next(as_iterator * i) {
    as_list_iterator_source * source = (as_list_iterator_source *) as_iterator_source(i);
    as_val * head = source->list->head;
    source->list = source->list->tail;
    return head;
}

static const int as_list_iterator_free(as_iterator * i) {
    free(i);
    return 0;
}

static const as_iterator_hooks as_list_iterator_hooks = {
    as_list_iterator_has_next,
    as_list_iterator_next,
    as_list_iterator_free,
};

static const as_val AS_LIST_VAL = {AS_LIST, as_list_freeval};