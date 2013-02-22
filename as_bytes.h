#pragma once

#include <sys/types.h>
#include <string.h>

#include "as_util.h"
#include "as_val.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_bytes_s as_bytes;

struct as_bytes_s {
    as_val _;
    bool value_is_malloc; 
    uint8_t * value;
    size_t len;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_val as_bytes_val;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_bytes *    as_bytes_init(as_bytes *, uint8_t *s, size_t len, bool is_malloc);
as_bytes *   as_bytes_new(uint8_t *, size_t len, bool is_malloc);

void           as_bytes_destroy(as_bytes *);
void    		as_bytes_val_destroy(as_val *v);

uint32_t as_bytes_val_hash(const as_val *);
char *as_bytes_val_tostring(const as_val *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline uint8_t * as_bytes_tobytes(const as_bytes * s) {
    return (s->value);
}

size_t as_bytes_len(as_bytes * s);

uint32_t as_bytes_hash(const as_bytes * s);

inline as_val * as_bytes_toval(const as_bytes * s) {
    return (as_val *)s;
}

inline as_bytes * as_bytes_fromval(const as_val * v) {
    return as_util_fromval(v, AS_BYTES, as_bytes);
}
