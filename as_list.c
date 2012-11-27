#include "as_list.h"
#include <stdbool.h>
#include <stdlib.h>

static const as_val AS_LIST_VAL;


as_list * as_list_new(void * source, const as_list_hooks * hooks) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    l->_ = AS_LIST_VAL;
    l->source = source;
    l->hooks = hooks;
    return l;
}

int as_list_free(as_list * l) {
    if ( l->hooks == NULL ) return 1;
    if ( l->hooks->free == NULL ) return 2;
    return l->hooks->free(l);
}

void * as_list_source(const as_list * l) {
    return l->source;
}

int as_list_append(as_list * l, as_val * v) {
    if ( l->hooks == NULL ) return 1;
    if ( l->hooks->append == NULL ) return 2;
    return l->hooks->append(l, v);
}

int as_list_prepend(as_list * l, as_val * v) {
    if ( l->hooks == NULL ) return 1;
    if ( l->hooks->prepend == NULL ) return 2;
    return l->hooks->prepend(l, v);
}

as_val * as_list_get(const as_list * l, const uint32_t i) {
    if ( l->hooks == NULL ) return NULL;
    if ( l->hooks->get == NULL ) return NULL;
    return l->hooks->get(l, i);
}

as_val * as_list_head(const as_list * l) {
    if ( l->hooks == NULL ) return NULL;
    if ( l->hooks->head == NULL ) return NULL;
    return l->hooks->head(l);
}

as_list * as_list_tail(const as_list * l) {
    if ( l->hooks == NULL ) return NULL;
    if ( l->hooks->tail == NULL ) return NULL;
    return l->hooks->tail(l);
}

as_iterator * as_list_iterator(const as_list * l) {
    if ( l->hooks == NULL ) return NULL;
    if ( l->hooks->iterator == NULL ) return NULL;
    return l->hooks->iterator(l);
}

as_val * as_list_toval(const as_list * l) {
    return (as_val *) l;
}

as_list * as_list_fromval(const as_val * v) {
    return as_val_type(v) == AS_LIST ? (as_list *) v : NULL;
}

static int as_list_freeval(as_val * v) {
    return as_val_type(v) == AS_LIST ? as_list_free((as_list *) v) : 1;
}

static const as_val AS_LIST_VAL = {AS_LIST, as_list_freeval};