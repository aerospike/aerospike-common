#pragma once

#include <stdlib.h>

/*
** an as_buffer is not a subtype of as_val and can't be used in any as_val functions,
** thus I suspect we need to remove it
*/

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_buffer_s as_buffer;

struct as_buffer_s {
    size_t  capacity;
    size_t  size;
    char *  data;
};

/******************************************************************************
 * INLINE FUNCTION DEFINITIONS
 ******************************************************************************/

int as_buffer_init(as_buffer * b);
void as_buffer_destroy(as_buffer * b);