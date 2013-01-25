#pragma once

#include "as_list.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_arraylist_s as_arraylist;
typedef struct as_arraylist_iterator_source_s as_arraylist_iterator_source;

struct as_arraylist_s {
    as_val **   elements;
    uint32_t    size;
    uint32_t    capacity;
    uint32_t    block_size;
};

struct as_arraylist_iterator_source_s {
    as_arraylist *  list;
    uint32_t        pos;
};

enum as_arraylist_status {
    AS_ARRAYLIST_OK         = 0,
    AS_ARRAYLIST_ERR_ALLOC  = 1,
    AS_ARRAYLIST_ERR_MAX    = 2
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_list_hooks      as_arraylist_list;
extern const as_iterator_hooks  as_arraylist_iterator;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_arraylist *    as_arraylist_init(as_arraylist *, uint32_t, uint32_t);
int               as_arraylist_destroy(as_arraylist *);

as_arraylist *    as_arraylist_new(uint32_t, uint32_t);
int               as_arraylist_free(as_arraylist *);