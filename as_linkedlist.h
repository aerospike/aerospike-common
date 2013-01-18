#pragma once

#include "as_list.h"
#include "as_val.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_linkedlist_s as_linkedlist;
typedef struct as_linkedlist_iterator_source_s as_linkedlist_iterator_source;

struct as_linkedlist_s {
    as_val *        head;
    as_linkedlist * tail;
};


struct as_linkedlist_iterator_source_s {
    const as_linkedlist * list;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_list_hooks      as_linkedlist_list;
extern const as_iterator_hooks  as_linkedlist_iterator;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_linkedlist_destroy(as_linkedlist *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline as_linkedlist * as_linkedlist_init(as_linkedlist * l, as_val * head, as_linkedlist * tail) {
    if ( !l ) return l;
    l->head = head;
    l->tail = tail;
    return l;
}

inline as_linkedlist * as_linkedlist_new(as_val * head, as_linkedlist * tail) {
    as_linkedlist * l = (as_linkedlist *) malloc(sizeof(as_linkedlist));
    return as_linkedlist_init(l, head, tail);
}

inline int as_linkedlist_free(as_linkedlist * l) {
    if ( !l ) return 0;
    as_linkedlist_destroy(l);
    free(l);
    return 0;
}

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cons(head,tail) \
    as_linkedlist_new((as_val *) head, (as_linkedlist *) tail)

