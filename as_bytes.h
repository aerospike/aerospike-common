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
    size_t len; // length of bytes allocated in value == size
    size_t capacity;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_bytes *    as_bytes_init(as_bytes *, uint8_t *s, size_t len, bool is_malloc);
as_bytes *    as_bytes_empty_init(as_bytes *, size_t len);
as_bytes *   as_bytes_new(uint8_t *, size_t len, bool is_malloc);
as_bytes *	 as_bytes_empty_new(size_t len);

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

// copy from as_bytes to a buf
int as_bytes_get(const as_bytes * s, int index, uint8_t *buf, int buf_len);

// copy from buf to as_bytes
int as_bytes_set(as_bytes * s, int index, const uint8_t *buf, int buf_len);

// create a new as_bytes, a substring of the source
as_bytes *as_bytes_slice_new(const as_bytes *src, int start_index, int end_index);

// create a new as_bytes, a substring of the source
as_bytes *as_bytes_slice_init(as_bytes *dst, const as_bytes *src, int start_index, int end_index);

// create a new as_bytes, a substring of the source
as_bytes *as_bytes_slice_new(const as_bytes *src, int start_index, int end_index);

// create a new as_bytes, a substring of the source
as_bytes *as_bytes_slice_init(as_bytes *dst, const as_bytes *src, int start_index, int end_index);

int as_bytes_append(as_bytes *v, const uint8_t *buf, int buf_len);

int as_bytes_append_bytes(as_bytes *s1, as_bytes *s2);

int as_bytes_delete(as_bytes *s, int pos, int len);

// changes length. Truncates if new length shorter.
// extends with 0 if new length longer.
int as_bytes_set_len(as_bytes *s, int len);

// TODO: remove bytes from the middle (trim/delete)

inline as_val * as_bytes_toval(const as_bytes * s) {
    return (as_val *)s;
}

inline as_bytes * as_bytes_fromval(const as_val * v) {
    return as_util_fromval(v, AS_BYTES, as_bytes);
}
