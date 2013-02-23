#include "as_buffer.h"
#include <stdio.h>

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
	if (b->data) { free(b->data); b->data = 0; }
    return;
} 
 
