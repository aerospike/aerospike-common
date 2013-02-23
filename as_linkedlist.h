#pragma once

#include "as_list.h"
#include "as_val.h"

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cons(head,tail) \
    as_linkedlist_new((as_val *) head, (as_linkedlist *) tail)

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_list_hooks      as_linkedlist_list_hooks;
extern const as_iterator_hooks  as_linkedlist_iterator_hooks;

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_linkedlist_source_s as_linkedlist_source;
typedef struct as_linkedlist_iterator_source_s as_linkedlist_iterator_source;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list *   as_linkedlist_init(as_list *, as_val *, as_list *);
as_list *   as_linkedlist_new(as_val *, as_list *);
void		as_linkedlist_destroy(as_list *); // call this
