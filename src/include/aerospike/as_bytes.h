/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <sys/types.h>
#include <string.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

/********
* CONSTANTS
*********/

// this section should use "proto.h" definitions
// in aerospike-common but that header is not available
// and is not yet consistant. Thus we will keep another copy
// which must be in sync.

typedef enum {
    AS_BYTES_TYPE_NULL = 0,
    AS_BYTES_TYPE_INTEGER = 1,
    AS_BYTES_TYPE_FLOAT = 2,
    AS_BYTES_TYPE_STRING = 3,
    AS_BYTES_TYPE_BLOB = 4,
    AS_BYTES_TYPE_TIMESTAMP = 5,
    AS_BYTES_TYPE_DIGEST = 6,
    AS_BYTES_TYPE_JAVA_BLOB = 7,
	AS_BYTES_TYPE_CSHARP_BLOB = 8,
	AS_BYTES_TYPE_PYTHON_BLOB = 9,
	AS_BYTES_TYPE_RUBY_BLOB = 10,
	AS_BYTES_TYPE_ERLANG_BLOB = 11,
	AS_BYTES_TYPE_APPEND = 12,
	AS_BYTES_TYPE_LUA_BLOB = 13,
	AS_BYTES_TYPE_JSON_BLOB = 14,
    AS_BYTES_TYPE_MAP = 15,
    AS_BYTES_TYPE_LIST = 16,
    AS_BYTES_TYPE_MAX = 17
} as_bytes_type;

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_bytes_s as_bytes;

struct as_bytes_s {
    as_val _;
    bool value_is_malloc; 
    as_bytes_type  type;
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

as_bytes_type as_bytes_get_type(const as_bytes * s);

void as_bytes_set_type(as_bytes *s, as_bytes_type t);

// copy from as_bytes to a buf
int as_bytes_get(const as_bytes * s, int index, uint8_t *buf, int buf_len);

// copy from buf to as_bytes
int as_bytes_set(as_bytes * s, int index, const uint8_t *buf, int buf_len);

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
