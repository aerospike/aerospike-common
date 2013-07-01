/******************************************************************************
 *	Copyright 2008-2013 by Aerospike.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy 
 *	of this software and associated documentation files (the "Software"), to 
 *	deal in the Software without restriction, including without limitation the 
 *	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 *	sell copies of the Software, and to permit persons to whom the Software is 
 *	furnished to do so, subject to the following conditions:
 * 
 *	The above copyright notice and this permission notice shall be included in 
 *	all copies or substantial portions of the Software.
 * 
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *	IN THE SOFTWARE.
 *****************************************************************************/

#pragma once

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <string.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

/**
 *	Types for `as_bytes.type`
 */
typedef enum as_bytes_type_e {

	/** 
	 *	Generic BLOB
	 */
	AS_BYTES_BLOB			= 4,

	/**
	 *	Serialized Java Object
	 */
	AS_BYTES_JAVA			= 7,

	/**
	 *	Serialized C# Object
	 */
	AS_BYTES_CSHARP		= 8,

	/**
	 *	Pickled Python Object
	 */
	AS_BYTES_PYTHON		= 9,

	/**
	 *	Marshalled Ruby Object
	 */
	AS_BYTES_RUBY		= 10,

	/**
	 *	Serialized Erlang Data
	 */
	AS_BYTES_ERLANG		= 11,

	/**
	 *	Serialized Lua Table
	 */
	AS_BYTES_LUA		= 13,

	/**
	 *	JSON Object
	 */
	AS_BYTES_JSON		= 14

} as_bytes_type;

/**
 *	Byte Array value
 *
 *	The `as_bytes` can be allocated on the stack or the heap.
 *	In either case, the `as_bytes` can be initialized with a value or
 *	be empty.
 *
 *	To initialize a stack allocated `as_bytes` with a value, use 
 *	`as_bytes_init()`
 *	
 *	@extends as_val
 */
typedef struct as_bytes_s {

	/**
	 *	@private
	 *	as_boolean is a subtype of as_val.
	 *	You can cast as_boolean to as_val.
	 */
	as_val _;

	/**
	 *	The number of bytes allocated to `as_bytes.value`.
	 */
	uint32_t capacity;

	/**
	 *	The number of bytes used by `as_bytes.value`.
	 */
	uint32_t size;

	/**
	 *	A sequnece of bytes. 
	 */
	uint8_t * value;

	/**
	 *	If true, then `as_bytes.value` will be freed when as_bytes_destroy()
	 *	is called.
	 */
	bool free;

	/**
	 *	The type of bytes.
	 */
	as_bytes_type type;

} as_bytes;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

/**
 *	Initializes a stack allocated `as_bytes`, filled with specified bytes.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t raw[10] = {0};
 *
 *	as_bytes bytes;
 *	as_bytes_init(&bytes, raw, 10, false);
 *	~~~~~~~~~~
 *
 *	@param bytes 	The bytes to initialize.
 *	@param value	The initial value.
 *	@param size		The number of bytes of the initial value.
 *	@param free		If true, then `as_bytes_destroy()` will free the value.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_init(as_bytes * bytes, uint8_t * value, uint32_t size, bool free);

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
as_bytes * as_bytes_init_empty(as_bytes * bytes, uint32_t capacity);

/**
 *	Creates a new heap allocated `as_bytes`, filled with specified bytes.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t raw[10] = {0};
 *
 *	as_bytes * bytes = as_bytes_new(raw, 10, false);
 *	~~~~~~~~~~
 *
 *	@param value	The initial value.
 *	@param size		The number of bytes of the initial value.
 *	@param free		If true, then `as_bytes_destroy()` will free the value.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_new(uint8_t * value, uint32_t size, bool free);

/**
 *	Creates a new heap allocated `as_bytes`, as an empty buffer of capacity size.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes * bytes = as_bytes_new_empty(10);
 *	~~~~~~~~~~
 *	
 *	@param capacity	The number of bytes to allocate.
 *
 *	@return On success, the initializes bytes. Otherwise NULL.
 */
as_bytes * as_bytes_new_empty(uint32_t capacity);

/**
 *	Destroy the `as_bytes` and release associated resources.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_destroy(bytes);
 *	~~~~~~~~~~
 *
 *	@param bytes	The bytes to destroy.
 */
inline void as_bytes_destroy(as_bytes * bytes) 
{
	as_val_destroy((as_val *) bytes;
}

/******************************************************************************
 *	VALUE FUNCTIONS
 *****************************************************************************/

/**
 *	The length of the buffer.
 *
 *	@param bytes The as_bytes to get the length of.
 *
 *	@return The number of bytes used.
 */
uint32_t as_bytes_len(as_bytes * bytes);

/**
 *	Get the type of bytes.
 *
 *	@param bytes The as_bytes to get the type of.
 *
 *	@return The type of bytes.
 */
as_bytes_type as_bytes_get_type(const as_bytes * bytes);

/**
 *	Set the type of bytes.
 *
 *	@param bytes	The as_bytes to get the type of.
 *	@param type		The type for the specified bytes.
 */
void as_bytes_set_type(as_bytes * bytes, as_bytes_type type);


/******************************************************************************
 *	GET AT INDEX
 *****************************************************************************/

/** 
 *	Copy into value up to size bytes from the given `as_bytes`, returning
 *	the number of bytes copied.
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
 *	@param size		The number of bytes to copy into the buffer.
 *
 *	@return The number of bytes copied in to value.
 */
uint32_t as_bytes_get(const as_bytes * bytes, uint32_t index, const uint8_t * value, uint32_t size);

/** 
 *	Read a single byte from the given gytes.
 *
 *	~~~~~~~~~~{.c}
 *	uint8_t value = as_bytes_get_byte(&bytes, 0);
 *	~~~~~~~~~~
 *
 *	@return An uint8_t value at the specified index.
 */
inline uint8_t as_bytes_get_byte(const as_bytes * bytes, uint32_t index)
{
	uint8_t value = 0;
	as_bytes_get(bytes, index, (uint8_t *) &value, 1);
	return value;
}

/** 
 *	Read an int16_t from the given bytes.
 *
 *	~~~~~~~~~~{.c}
 *	int16_t value = as_bytes_get_int16(&bytes, 0);
 *	~~~~~~~~~~
 *
 *	@return On success, the int16_t value at the specified index. Otherwise INT16_MIN.
 */
inline int16_t as_bytes_get_int16(const as_bytes * bytes, uint32_t index)
{
	int16_t value = 0;
	as_bytes_get(bytes, index, (uint8_t *) &value, 2);
	return value;
}

/** 
 *	Read an int32_t from the given bytes.
 *
 *	~~~~~~~~~~{.c}
 *	int32_t value = as_bytes_get_int32(&bytes, 0);
 *	~~~~~~~~~~
 *
 *	@return On success, the int32_t value at the specified index. Otherwise INT32_MIN.
 */
inline int32_t as_bytes_get_int32(const as_bytes * bytes, uint32_t index)
{
	int32_t value = 0;
	as_bytes_get(bytes, index, (uint8_t *) &value, 4);
	return value;
}

/** 
 *	Read an int64_t from the given bytes.
 *
 *	~~~~~~~~~~{.c}
 *	int64_t value = as_bytes_get_int64(&bytes, 0);
 *	~~~~~~~~~~
 *
 *	@return On success, the int64_t value at the specified index. Otherwise INT64_MIN.
 */
inline int64_t as_bytes_get_int64(const as_bytes * bytes, uint32_t index)
{
	int64_t value = 0;
	as_bytes_get(bytes, index, (uint8_t *) &value, 8);
	return value;
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
 *	@param bytes	The bytes to write to.
 *	@param index	The position to write to.
 *	@param value 	The buffer to read from.
 *	@param size		The number of bytes to read from the value.
 *	
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_set(as_bytes * bytes, uint32_t index, const uint8_t * value, uint32_t size);

/**
 *	Set a byte at given index.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_byte(&bytes, 'a');
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_set_byte(as_bytes * bytes, uint32_t index, uint8_t value)
{
	return as_bytes_set(bytes, index, (uint8_t *) &value, 1);
}

/**
 *	Set a byte at given index.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_byte(&bytes, 'a');
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_set_int16(as_bytes * bytes, uint32_t index, int16_t value)
{
	return as_bytes_set(bytes, index, (uint8_t *) &value, 2);
}

/**
 *	Set a byte at given index.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_byte(&bytes, 'a');
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_set_int32(as_bytes * bytes, uint32_t index, int32_t value)
{
	return as_bytes_set(bytes, index, (uint8_t *) &value, 4);
}

/**
 *	Set a byte at given index.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_byte(&bytes, 'a');
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_set_int64(as_bytes * bytes, uint32_t index, int64_t value)
{
	return as_bytes_set(bytes, index, (uint8_t *) &value, 8);
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
 *	@param bytes	The bytes to append to.
 *	@param value	The buffer to read from.
 *	@param size		The number of bytes to read from the value.
 *
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_append(as_bytes * bytes, const uint8_t * value, uint32_t size);

/**
 *	Append a uint8_t (byte).
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_byte(&bytes, 'a');
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_append_byte(as_bytes * bytes, uint8_t value)
{
	return as_bytes_append(bytes, index, (uint8_t *) &value, 1);
}

/**
 *	Append an int16_t value.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_int16(&bytes, 123);
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_append_int16(as_bytes * bytes, int16_t value)
{
	return as_bytes_append(bytes, index, (uint8_t *) &value, 2);
}

/**
 *	Append an int32_t value.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_int32(&bytes, 123);
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_append_int32(as_bytes * bytes, int32_t value)
{
	return as_bytes_append(bytes, index, (uint8_t *) &value, 4);
}

/**
 *	Append an int64_t value.
 *
 *	~~~~~~~~~~{.c}
 *	as_bytes_append_int64(&bytes, 123;
 *	~~~~~~~~~~
 *
 *	@return On success, true. Otherwise an error occurred.
 */
inline bool as_bytes_append_int64(as_bytes * bytes, int64_t value)
{
	return as_bytes_append(bytes, index, (uint8_t *) &value, 8);
}

/******************************************************************************
 *	MODIFIES BUFFER
 *****************************************************************************/

/**
 *	Truncate the bytes' buffer. The size specifies the number of bytes to 
 *	remove from the end of the buffer. 
 *
 *	This means, if the buffer has size of 100, and we truncate 10, then
 *	the remaining size is 90. 

 *	Truncation does not modify the capacity of the buffer.
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
bool as_bytes_truncate(as_bytes * bytes, uint32_t n);

/**
 *	Ensure the bytes buffer can handle `n` additional bytes.
 *
 *	Using the current size, we see if `size + n` is within the capcity of 
 *	the bytes' buffer. If so, then return true.
 *	
 *	If `resize` is true and `size + n` exceeds the capacity of the bytes's 
 *	buffer, then resize the capacity of the buffer by `n` bytes. If the buffer
 *	was heap allocated, then `realloc()` will be used to resize. If the buffer
 *	was stack allocated, it will be converted to a heap allocated buffer using
 *	malloc() and then its contents will be copied into the new heap allocated 
 *	buffer.
 *
 *	If `resize` is false, and if the capacity is not sufficient, then return
 *	false.
 *	
 *	~~~~~~~~~~{.c}
 *	as_bytes_ensure(&bytes, 100, true);
 *	~~~~~~~~~~
 *	
 *	@param bytes	The bytes to ensure the capacity of.
 *	@param n		The number of additional bytes to ensure bytes can handle.
 *	@param resize	If true and capacity is not sufficient, then resize the buffer.
 *
 *	@return On success, true. Otherwise an error occurred.
 */
bool as_bytes_ensure(as_bytes * bytes, uint32_t n, bool resize);


/**
 *	Get the bytes value.
 *
 *	@deprecated Use as_bytes_get() instead.
 */
inline uint8_t * as_bytes_tobytes(const as_bytes * b, uint32_t * size) 
{
	if ( !bytes ) return NULL;

	if ( size ) {
		*size = bytes->size;
	}

	return bytes->value;
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_bytes_toval(const as_bytes * b) 
{
	return (as_val *) b;
}

/**
 *	Convert from an as_val.
 */
inline as_bytes * as_bytes_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_BYTES, as_bytes);
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_bytes_val_destroy(as_val * v);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_bytes_val_hashcode(const as_val * v);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_bytes_val_tostring(const as_val * v);
