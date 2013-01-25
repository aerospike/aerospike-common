#pragma once

#include "as_list.h"
#include "as_val.h"

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cons(head,tail) \
    as_linkedlist_new((as_val *) head, (as_linkedlist *) tail)

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

as_linkedlist *   as_linkedlist_init(as_linkedlist *, as_val *, as_linkedlist *);
int               as_linkedlist_destroy(as_linkedlist *);

as_linkedlist *   as_linkedlist_new(as_val *, as_linkedlist *);
int               as_linkedlist_free(as_linkedlist *);
