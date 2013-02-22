#include <stdlib.h>
#include <string.h>
#include <cf_alloc.h>

#include "as_bytes.h"

extern inline uint8_t * as_bytes_tobytes(const as_bytes * s);
extern inline as_val * as_bytes_toval(const as_bytes * s);
extern inline as_bytes * as_bytes_fromval(const as_val * v);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_bytes * as_bytes_init(as_bytes * v, uint8_t * s, size_t len, bool is_malloc) {
    as_val_init(&v->_, AS_BYTES, false /*is_malloc*/);
    v->value_is_malloc = is_malloc;
    v->value = s;
    v->len = len;
    return v;
}

as_bytes * as_bytes_new(uint8_t * s, size_t len, bool is_malloc) {
    as_bytes * v = (as_bytes *) malloc(sizeof(as_bytes));
    as_val_init(&v->_, AS_BYTES, true /*is_malloc*/);
    v->value_is_malloc = is_malloc;
    v->value = s;
    v->len = len;
    return v;
}

void as_bytes_destroy(as_bytes * s) {
    if ( s->value_is_malloc && s->value ) free(s->value);
}

void as_bytes_val_destroy(as_val * v) {
   as_bytes_destroy( (as_bytes *)v );
}

size_t as_bytes_len(as_bytes * s) {
	return(s->len);
}

uint32_t as_bytes_hash(const as_bytes * s) {
    if (s->value == NULL) return(0);
    uint32_t hash = 0;
    int c;
    uint8_t * str = s->value;
    while ( (c = *str++) ) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

uint32_t as_bytes_val_hash(const as_val * v) {
    return as_bytes_hash((as_bytes *) v);
}

char * as_bytes_val_tostring(const as_val * v) {
    as_bytes * s = (as_bytes *) v;
    if (s->value == NULL) return(NULL);
    size_t sl = as_bytes_len(s);
    size_t st = 3 + sl;
    char * str = (char *) malloc(sizeof(char) * st);
    *(str + 0) = '\"';
    memcpy(str + 1, s->value, s->len);
    *(str + 1 + sl) = '\"';
    *(str + 1 + sl + 1) = '\0';
    return str;
}
