/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <aerospike/as_bytes.h>
#include <citrusleaf/alloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

/**
 *	Decode an integer in variable 7-bit format.
 *	The high bit indicates if more bytes are used.
 *
 *	~~~~~~~~~~{.c}
 *	uint32_t value = 0;
 *	uint32_t sz = as_bytes_get_var_int(&bytes, 0, &value);
 *	if ( sz == 0 ) {
 *		// sz == 0, means that an error occurred
 *	}
 *	~~~~~~~~~~
 *
 *	@return The number of bytes copied in to value.
 *
 *	@relatesalso as_bytes
 */
uint32_t as_bytes_get_var_int(const as_bytes * bytes, uint32_t index, uint32_t * value)
{
	uint8_t* begin = bytes->value + index;
	uint8_t* p = begin;
	uint32_t val = 0;
	uint32_t shift = 0;
	uint8_t b;
	
	do {
		b = *p++;
		val |= (b & 0x7F) << shift;
		shift += 7;
	} while ((b & 0x80));
	
	*value = val;
	return (uint32_t)(p - begin);
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

/**
 *	Encode an integer in 7-bit format.
 *	The high bit indicates if more bytes are used.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_set_var_int(&bytes, 0, 36);
 *	~~~~~~~~~~
 *
 *	The `bytes` must be sufficiently sized for the data being written.
 *	To ensure the `bytes` is allocated sufficiently, you will need to call
 *	`as_bytes_ensure()`.
 *
 *	@return The number of bytes copied into byte array.
 *
 *	@relatesalso as_bytes
 */
uint32_t as_bytes_set_var_int(const as_bytes * bytes, uint32_t index, uint32_t value)
{
	uint8_t* begin = bytes->value + index;
	uint8_t* end = bytes->value + bytes->capacity;
	uint8_t* p = begin;
	
	while (p < end && value >= 0x80) {
		*p++ = (uint8_t)(value | 0x80);
		value >>= 7;
	}
	
	if (p < end) {
		*p++ = (uint8_t)value;
		return (uint32_t)(p - begin);
	}
	return 0;
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
