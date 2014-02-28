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

#include <citrusleaf/alloc.h>

#include <aerospike/as_bytes.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void as_bytes_destroy(as_bytes * s);

extern inline uint32_t as_bytes_size(const as_bytes * bytes);
extern inline uint32_t as_bytes_capacity(const as_bytes * bytes);

extern inline as_bytes_type as_bytes_get_type(const as_bytes * bytes);
extern inline void as_bytes_set_type(as_bytes * bytes, as_bytes_type type);

extern inline uint8_t * as_bytes_getorelse(const as_bytes * bytes, uint8_t * fallback);
extern inline uint8_t * as_bytes_get(const as_bytes * bytes);

extern inline uint32_t as_bytes_get_byte(const as_bytes * bytes, uint32_t index, uint8_t * value);
extern inline uint32_t as_bytes_get_int16(const as_bytes * bytes, uint32_t index, int16_t * value);
extern inline uint32_t as_bytes_get_int32(const as_bytes * bytes, uint32_t index, int32_t * value);
extern inline uint32_t as_bytes_get_int64(const as_bytes * bytes, uint32_t index, int64_t * value);

extern inline bool as_bytes_set_byte(as_bytes * bytes, uint32_t index, uint8_t value);
extern inline bool as_bytes_set_int16(as_bytes * bytes, uint32_t index, int16_t value);
extern inline bool as_bytes_set_int32(as_bytes * bytes, uint32_t index, int32_t value);
extern inline bool as_bytes_set_int64(as_bytes * bytes, uint32_t index, int64_t value);

extern inline bool as_bytes_append_byte(as_bytes * bytes, uint8_t value);
extern inline bool as_bytes_append_int16(as_bytes * bytes, int16_t value);
extern inline bool as_bytes_append_int32(as_bytes * bytes, int32_t value);
extern inline bool as_bytes_append_int64(as_bytes * bytes, int64_t value);

extern inline uint8_t * as_bytes_tobytes(const as_bytes * s, uint32_t * size);

extern inline as_val * as_bytes_toval(const as_bytes * s);
extern inline as_bytes * as_bytes_fromval(const as_val * v);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const char hex_chars[] = "0123456789ABCDEF";

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

static inline as_bytes * as_bytes_cons(
	as_bytes * bytes, bool free, 
	uint32_t capacity, uint32_t size, uint8_t * value, 
	bool value_free, as_bytes_type type)
{
	if ( !bytes ) return bytes;

    as_val_cons((as_val *) bytes, AS_BYTES, free);
    bytes->capacity = capacity;
    bytes->size = size;
    bytes->value = value;
    bytes->free = value_free;
    bytes->type = AS_BYTES_BLOB;

    if ( value == NULL && size == 0 && capacity > 0 ) {
	    bytes->value = cf_calloc(capacity, sizeof(uint8_t));
    }
    
	return bytes;
}

/**
 *	Initializes a stack allocated `as_bytes`, as an empty buffer of capacity size.
 *	
 *	~~~~~~~~~~{.c}
 *	as_bytes bytes;
 *	as_bytes_init_empty(&bytes, 10);
 *	~~~~~~~~~~
 *
 *	@param bytes 	The bytes to initialize.
 *	@param capacity	The number of bytes to allocate on the heap.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_init(as_bytes * bytes, uint32_t capacity)
{
	return as_bytes_cons(bytes, false, capacity, 0, NULL, true, AS_BYTES_BLOB);
}

/**
 *	Initializes a stack allocated `as_bytes`, wrapping the given buffer.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t raw[10] = {0};
 *
 *	as_bytes bytes;
 *	as_bytes_init_wrap(&bytes, raw, 10, false);
 *	~~~~~~~~~~
 *	
 *	@param bytes 	The bytes to initialize.
 *	@param value	The initial value.
 *	@param size		The number of bytes of the initial value.
 *	@param free		If true, then `as_bytes_destroy()` will free the value.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_init_wrap(as_bytes * bytes, uint8_t * value, uint32_t size, bool free)
{
	return as_bytes_cons(bytes, false, size, size, value, free, AS_BYTES_BLOB);
}

/**
 *	Creates a new heap allocated `as_bytes`, as an empty buffer of capacity size.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes * bytes = as_bytes_new(10);
 *	~~~~~~~~~~
 *	
 *	@param capacity	The number of bytes to allocate.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_new(uint32_t capacity)
{
    as_bytes * bytes = (as_bytes *) cf_malloc(sizeof(as_bytes));
    if ( !bytes ) return bytes;
	return as_bytes_cons(bytes, true, capacity, 0, NULL, true, AS_BYTES_BLOB);
}

/**
 *	Creates a new heap allocated `as_bytes`, wrapping the given buffer.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t raw[10] = {0};
 *
 *	as_bytes * bytes = as_bytes_new_wrap(raw, 10, false);
 *	~~~~~~~~~~
 *
 *	@param value	The initial value.
 *	@param size		The number of bytes of the initial value.
 *	@param free		If true, then `as_bytes_destroy()` will free the value.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_new_wrap(uint8_t * value, uint32_t size, bool free)
{
    as_bytes * bytes = (as_bytes *) cf_malloc(sizeof(as_bytes));
    if ( !bytes ) return bytes;
	return as_bytes_cons(bytes, true, size, size, value, free, AS_BYTES_BLOB);
}

/******************************************************************************
 *	GET AT INDEX
 *****************************************************************************/

/** 
 *	Copy into value up to size bytes from the given `as_bytes`, returning
 *	the number of bytes copied. The number of bytes copied may be smaller
 *	than `size`, if `index + size` is larger than the buffer's size.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t value[3] = {0};
 *
 *	uint32_t size = as_bytes_get(&bytes, 0, value, 3);
 *	~~~~~~~~~~
 *
 *	@param bytes	The bytes to read from.
 *	@param index	The positing in bytes to read from.
 *	@param value	The byte buffer to copy into.
 *	@param size		The maximum number of bytes to copy into the buffer.
 *
 *	@return The number of bytes copied in to value.
 */
uint32_t as_bytes_copy(const as_bytes * bytes, uint32_t index, uint8_t * value, uint32_t size)
{
	uint32_t sz = index + size > bytes->size ? 0 : size;
	if ( sz == 0 ) return sz;
	memcpy(value, &bytes->value[index], sz);
    return sz;
}

/******************************************************************************
 *	SET AT INDEX
 *****************************************************************************/

/**
 *	Copy raw bytes of given size into the given `as_bytes` starting at
 *	specified index.
 *	
 *	~~~~~~~~~~{.c}
 *	as_bytes_set(&bytes, 0, (uint8_t[]){'a','b','c'}, 3);
 *	~~~~~~~~~~
 *
 *	The `bytes` must be sufficiently sized for the data being written.
 *	To ensure the `bytes` is allocated sufficiently, you will need to call
 *	`as_bytes_ensure()`.
 *
 *	@param bytes	The bytes to write to.
 *	@param index	The position to write to.
 *	@param value 	The buffer to read from.
 *	@param size		The number of bytes to read from the value.
 *	
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_set(as_bytes * bytes, uint32_t index, const uint8_t * value, uint32_t size)
{
    if ( index + size > bytes->capacity ) return false;
    memcpy(&bytes->value[index], value, size);
    if ( index + size > bytes->size ) {
    	bytes->size = index + size;
    }
    return true;
}

/******************************************************************************
 *	APPEND TO THE END
 *****************************************************************************/

/**
 *	Append raw bytes of given size.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t value[3] = {'a','b','c'};
 *
 *	as_bytes_append(&bytes, value, 3);
 *	~~~~~~~~~~
 *
 *	The `bytes` must be sufficiently sized for the data being written.
 *	To ensure the `bytes` is allocated sufficiently, you will need to call
 *	`as_bytes_ensure()`.
 *	
 *	@param bytes	The bytes to append to.
 *	@param value	The buffer to read from.
 *	@param size		The number of bytes to read from the value.
 *
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_append(as_bytes * bytes, const uint8_t * value, uint32_t size)
{
	return as_bytes_set(bytes, bytes->size, value, size);
}

/******************************************************************************
 *	MODIFIES BUFFER
 *****************************************************************************/

/**
 *	Truncate the buffer. The size specifies the number of bytes to remove from the
 *	end of the buffer. 
 *
 *	This means, if the buffer has size of 100, and we truncate 10, then
 *	the remaining size is 90. Truncation does not modify the capacity of the 
 *	buffer.
 *	
 *	~~~~~~~~~~{.c}
 *	as_bytes_truncate(&bytes, 10);
 *	~~~~~~~~~~
 *
 *	@param bytes	The bytes to truncate.
 *	@param n		The number of bytes to remove from the end.
 *	
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_truncate(as_bytes * bytes, uint32_t n)
{
	if ( n > bytes->size ) return false;
	bytes->size = bytes->size - n;
	return true;
}

/**
 *	Ensure the bytes buffer can handle `capacity` bytes.
 *		
 *	If `resize` is true and `capacity` exceeds the capacity of the bytes's 
 *	buffer, then resize the capacity of the buffer to `capacity` bytes. If the 
 *	buffer was heap allocated, then `cf_realloc()` will be used to resize. If the 
 *	buffer was stack allocated, it will be converted to a heap allocated buffer 
 *	using cf_malloc() and then its contents will be copied into the new heap 
 *	allocated  buffer.
 *
 *	If `resize` is false, and if the capacity is not sufficient, then return
 *	false.
 *	
 *	~~~~~~~~~~{.c}
 *	as_bytes_ensure(&bytes, 100, true);
 *	~~~~~~~~~~
 *	
 *	@param bytes	The bytes to ensure the capacity of.
 *	@param capacity	The total number of bytes to ensure bytes can handle.
 *	@param resize	If true and capacity is not sufficient, then resize the buffer.
 *
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_ensure(as_bytes * bytes, uint32_t capacity, bool resize)
{
	if ( capacity <= bytes->capacity ) return true;
	if ( !resize ) return false;

	uint8_t * buffer = NULL;

	if ( bytes->free ) {
		// this is a previously cf_malloc'd value
		buffer = cf_realloc(bytes->value, capacity);
		if ( !buffer ) {
			// allocation failed, so return false.
			return false;
		}
	}
	else {
		// this is a previously stack alloc'd value
		buffer = cf_malloc(capacity);
		if ( !buffer ) {
			// allocation failed, so return false.
			return false;
		}
		// copy the bytes
		memcpy(buffer, bytes->value, bytes->size);
	}

	bytes->free = true;
	bytes->value = buffer;
	bytes->capacity = capacity;

	return true;
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

void as_bytes_val_destroy(as_val * v)
{
    as_bytes * b = as_bytes_fromval(v);
    if ( b && b->free && b->value ) {
        cf_free(b->value);
    }
}

uint32_t as_bytes_val_hashcode(const as_val * v)
{
    as_bytes * bytes = as_bytes_fromval(v);
    if ( bytes == NULL || bytes->value == NULL ) return 0;
    uint32_t hash = 0;
    uint8_t * buf = bytes->value;
    int len = bytes->size;
    while ( --len ) {
        int b = *buf++;
        hash = b + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

char * as_bytes_val_tostring(const as_val * v)
{
    as_bytes * bytes = as_bytes_fromval(v);
    if ( !bytes || !bytes->value || !bytes->size ) {
    	return NULL;
    }

    uint8_t * 	s = bytes->value;
    uint32_t 	sl = bytes->size;
    size_t		st = (4 * sl) + 3;
    char * 		str = (char *) cf_malloc(st);

    if ( !str ) {
    	return NULL;
    }
    
    int j=0;
    for ( int i=0; i < sl; i++ ) {
        str[j] = hex_chars[ s[i] >> 4 ];
        str[j+1] = hex_chars[ s[i] & 0xf ];
        str[j+2] = ' ';
        j += 3;
    }
    j--; // chomp
    
    str[j] = 0;
    
    return str;
}
