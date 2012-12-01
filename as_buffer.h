#pragma once

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

inline int as_buffer_init(as_buffer * b) {
    b->capacity = 0;
    b->size = 0;
    b->data = NULL;
    return 0;
}

inline int as_buffer_free(as_buffer * b) {
    free(b->data);
    b->data = NULL;
    return 0;
}