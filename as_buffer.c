#include "as_buffer.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

int as_buffer_init(as_buffer * b) {
    b->capacity = 0;
    b->size = 0;
    b->data = NULL;
    return 0;
}

void as_buffer_destroy(as_buffer * b) {
    return;
} 
 
