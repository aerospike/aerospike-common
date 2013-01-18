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
    bool        shadow;
};

struct as_arraylist_iterator_source_s {
    as_arraylist *  list;
    uint32_t        pos;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_list_hooks      as_arraylist_list;
extern const as_iterator_hooks  as_arraylist_iterator;

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/


inline as_arraylist * as_arraylist_init(as_arraylist * a, uint32_t capacity, uint32_t block_size) {
    if ( !a ) return a;
    capacity = capacity == 0 ? 8 : capacity;
    a->elements = (as_val **) malloc(sizeof(as_val *) * capacity);
    a->size = 0;
    a->capacity = capacity;
    a->block_size = block_size;
    a->shadow = false;
    return a;
}

inline as_arraylist * as_arraylist_new(uint32_t capacity, uint32_t block_size) {
    as_arraylist * a = (as_arraylist *) malloc(sizeof(as_arraylist));
    return as_arraylist_init(a, capacity, block_size);
}

inline int as_arraylist_destroy(as_arraylist * a) {
    if ( !a ) return 0;
    if ( !a->shadow ) {
        for (int i = 0; i < a->size; i++ ) {
            as_val_free(a->elements[i]);
            a->elements[i] = NULL;
        }
    }
    free(a->elements);
    a->elements = NULL;
    a->size = 0;
    a->capacity = 0;
    return 0;
}

inline int as_arraylist_free(as_arraylist * a) {
    if ( !a ) return 0;
    as_arraylist_destroy(a);
    free(a);
    return 0;
}
