#pragma once

#include "as_list.h"
#include "as_val.h"

/**
 * Create a linked-list backed list
 * @param the head of the list
 * @param the tail of the list
 * @return a new as_list
 */
as_list * as_linkedlist_new(as_val *, as_list *);


#define cons(head,tail) \
    as_linkedlist_new((as_val *) head, (as_list *) tail)

