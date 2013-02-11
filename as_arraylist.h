#pragma once

#include "as_list.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_arraylist_source_s as_arraylist_source;
typedef struct as_arraylist_iterator_source_s as_arraylist_iterator_source;

enum as_arraylist_status {
    AS_ARRAYLIST_OK         = 0,
    AS_ARRAYLIST_ERR_ALLOC  = 1,
    AS_ARRAYLIST_ERR_MAX    = 2
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_list_hooks      as_arraylist_list_hooks;
extern const as_iterator_hooks  as_arraylist_iterator_hooks;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list *    	as_arraylist_init(as_list *, uint32_t, uint32_t);
as_list *    	as_arraylist_new(uint32_t, uint32_t);
int               as_arraylist_destroy(as_list *);
