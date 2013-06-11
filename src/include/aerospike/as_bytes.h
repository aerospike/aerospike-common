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

/******************************************************************************
 * TYPES
 *****************************************************************************/

/**
 * Types of as_bytes
 */
enum as_bytes_type_e {
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
};

typedef enum as_bytes_type_e as_bytes_type;

/**
 * Represents a sequence of bytes
 */
struct as_bytes_s {
	as_val          _;
	as_bytes_type   type;
	bool            free;
	uint32_t        capacity;
	uint32_t        len;
	uint8_t *       value;
};

typedef struct as_bytes_s as_bytes;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initializes a stack allocated as_bytes, filled with specified bytes.
 */
as_bytes * as_bytes_init(as_bytes * b, uint8_t * raw, uint32_t len, bool free);

/**
 * Initializes a stack allocated as_bytes, as an empty buffer of len size.
 */
as_bytes * as_bytes_empty_init(as_bytes * b, uint32_t len);

/**
 * Creates a new heap allocated as_bytes, filled with specified bytes.
 */
as_bytes * as_bytes_new(uint8_t * raw, uint32_t len, bool free);

/**
 * Creates a new heap allocated as_bytes, as an empty buffer of len size.
 */
as_bytes * as_bytes_empty_new(uint32_t len);

/**
 * The length of the buffer.
 */
uint32_t as_bytes_len(as_bytes * s);

/**
 * Get the type of bytes.
 */
as_bytes_type as_bytes_get_type(const as_bytes * b);

/**
 * Set the type of bytes.
 */
void as_bytes_set_type(as_bytes * b, as_bytes_type t);

/** 
 * Copy from as_bytes to a buf
 */
int as_bytes_get(const as_bytes * b, uint32_t index, uint8_t * buf, uint32_t len);

/**
 * Copy from buf to as_bytes
 */
int as_bytes_set(as_bytes * b, uint32_t index, const uint8_t * buf, uint32_t len);

/**
 * Creates a new as_bytes from a slice of the source.
 */
as_bytes * as_bytes_slice_new(const as_bytes * b, uint32_t start, uint32_t end);

/**
 * Initializes an as_bytes from a slice of the source.
 */
as_bytes * as_bytes_slice_init(as_bytes * dest, const as_bytes * src, uint32_t start, uint32_t end);

/**
 * Append specified bytes
 */
int as_bytes_append(as_bytes * b, const uint8_t * buf, uint32_t buf_len);

/**
 * Append another as_bytes.
 */
int as_bytes_append_bytes(as_bytes * b, as_bytes * s);

/**
 * Remove bytes from pos of given len.
 */
int as_bytes_delete(as_bytes * b, uint32_t pos, uint32_t len);

/**
 * Changes length of the buffer. 
 * If the new length is shorter than the current length, then the buffer is truncated.
 * If the new length is longer than the current length, then the buffer is expanded and new 
 * space is fille with 0 (zero).
 */
int as_bytes_set_len(as_bytes *s, uint32_t len);

/**
 * PRIVATE:
 * Internal helper function for destroying an as_val.
 */
void as_bytes_val_destroy(as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_bytes_val_hashcode(const as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the string representation of an as_val.
 */
char * as_bytes_val_tostring(const as_val * v);

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

/**
 * Destroy the as_bytes and associated resources.
 */
inline void as_bytes_destroy(as_bytes * b) 
{
	as_val_val_destroy( (as_val *) b );
}

/**
 * Get the bytes value.
 */
inline uint8_t * as_bytes_tobytes(const as_bytes * b) 
{
	return b ? b->value : NULL;
}

/******************************************************************************
 * CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 * Convert to an as_val.
 */
inline as_val * as_bytes_toval(const as_bytes * b) 
{
	return (as_val *) b;
}

/**
 * Convert from an as_val.
 */
inline as_bytes * as_bytes_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_BYTES, as_bytes);
}
