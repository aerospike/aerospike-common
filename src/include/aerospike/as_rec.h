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

#include <aerospike/as_integer.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>
#include <aerospike/as_string.h>
#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

struct as_rec_hooks_s;

/**
 *	Record Interface
 *
 *	To use the record interface, you will need to create an instance 
 *	via one of the implementations.
 */
typedef struct as_rec_s {

	/**
	 *	@private
	 *	as_boolean is a subtype of as_val.
	 *	You can cast as_boolean to as_val.
	 */
	as_val _;

	/**
	 *	Data provided by the implementation of `as_rec`.
	 */
	void * data;

	/**
	 *	Hooks provided by the implementation of `as_rec`.
	 */
	const struct as_rec_hooks_s * hooks;

} as_rec;

/**
 *	Record Hooks.
 *
 *	An implementation of `as_rec` should provide implementations for each
 *	of the hooks.
 */
typedef struct as_rec_hooks_s {

	/**
	 *	Destroy the record.
	 */
	bool (* destroy)(as_rec * rec);

	/**
	 *	Get the hashcode of the record.
	 */
	uint32_t (* hashcode)(const as_rec * rec);

	/**
	 *	Get the value of the bin in the record.
	 */
	as_val * (* get)(const as_rec * rec, const char * name);

	/**
	 *	Set the value of the bin in the record.
	 */
	int (* set)(const as_rec * rec, const char * name, const as_val * value);

	/**
	 *	Remove the bin from the record.	
	 */
	int (* remove)(const as_rec * rec, const char * bin);

	/**
	 *	Get the ttl value of the record.
	 */
	uint32_t (* ttl)(const as_rec  * rec);

	/**
	 *	Get the generation value of the record.
	 */
	uint16_t (* gen)(const as_rec * rec);

	/**
	 *	Get the number of bins of the record.
	 */
	uint16_t (* numbins)(const as_rec * rec);

	/**
	 *	Get the digest of the record.
	 */
	as_bytes * (* digest)(const as_rec * rec);

	/**
	 *	Set flags on a bin.
	 */
	int (* set_flags)(const as_rec * rec, const char * bin, uint8_t flags);

	/**
	 *	Set the type of record.
	 */
	int (* set_type)(const as_rec * rec,  uint8_t type);

} as_rec_hooks;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Utilized by subtypes of as_rec to initialize the parent.
 *
 *	@param rec		The record to initialize
 *	@param free 	If TRUE, then as_rec_destory() will free the record.
 *	@param data		Data for the map.
 *	@param hooks	Implementaton for the map interface.
 *	
 *	@return The initialized as_map on success. Otherwise NULL.
 */
as_rec * as_rec_cons(as_rec * rec, bool free, void * data, const as_rec_hooks * hooks);

/**
 *	Initialize a stack allocated record.
 *
 *	@param rec		Stack allocated record to initialize.
 *	@param data		Data for the record.
 *	@param hooks	Implementaton for the record interface.
 *	
 *	@return On success, the initialized record. Otherwise NULL.
 */
as_rec * as_rec_init(as_rec * rec, void * data, const as_rec_hooks * hooks);

/**
 *	Create and initialize a new heap allocated record.
 *	
 *	@param data		Data for the record.
 *	@param hooks	Implementaton for the record interface.
 *	
 *	@return On succes, a new record. Otherwise NULL.
 */
as_rec * as_rec_new(void * data, const as_rec_hooks * hooks);

/**
 *	Destroy the record.
 */
inline void as_rec_destroy(as_rec * rec) 
{
	as_val_destroy((as_val *) rec);
}

/******************************************************************************
 *	INLINE FUNCTIONS
 ******************************************************************************/

/**
 *	Get the data source for the record.
 */
inline void * as_rec_source(const as_rec * rec) 
{
	return rec ? rec->data : NULL;
}

/**
 *	Remove a bin from a record.
 *
 *	@param rec		The record to remove the bin from.
 *	@param name 	The name of the bin to remove.
 *
 *	@return 0 on success, otherwise an error occurred.
 */
inline int as_rec_remove(const as_rec * rec, const char * name) 
{
	return as_util_hook(remove, 1, rec, name);
}

/**
 *	Get the ttl for the record.
 */
inline uint32_t as_rec_ttl(const as_rec * rec) 
{
	return as_util_hook(ttl, 0, rec);
}

/**
 *	Get the generation of the record
 */
inline uint16_t as_rec_gen(const as_rec * rec) 
{
	return as_util_hook(gen, 0, rec);
}

/**
 *	Get the number of bins in the record.
 */
inline uint16_t as_rec_numbins(const as_rec * rec) 
{
	return as_util_hook(numbins, 0, rec);
}

/**
 *	Get the digest of the record.
 */
inline as_bytes * as_rec_digest(const as_rec * rec) 
{
	return as_util_hook(digest, 0, rec);
}

/**
 *	Set flags on a bin.
 */
inline int  as_rec_set_flags(const as_rec * rec, const char * name, uint8_t flags) 
{
	return as_util_hook(set_flags, 0, rec, name, flags);
}

/**
 *	Set the record type.
 */
inline int as_rec_set_type(const as_rec * rec, uint8_t rec_type) 
{
	return as_util_hook(set_type, 0, rec, rec_type);
}

/******************************************************************************
 *	BIN GETTER FUNCTIONS
 ******************************************************************************/

/**
 *	Get a bin's value.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline as_val * as_rec_get(const as_rec * rec, const char * name) 
{
	return as_util_hook(get, NULL, rec, name);
}

/**
 *	Get a bin's value as an int64_t.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise 0.
 */
inline int64_t as_rec_get_int64(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	as_integer * i = as_integer_fromval(v);
	return i ? as_integer_toint(i) : 0;
}

/**
 *	Get a bin's value as a NULL terminated string.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline char * as_rec_get_str(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	as_string * s = as_string_fromval(v);
	return s ? as_string_tostring(s) : 0;
}

/**
 *	Get a bin's value as an as_integer.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline as_integer * as_rec_get_integer(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	return as_integer_fromval(v);
}

/**
 *	Get a bin's value as an as_string.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name		The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline as_string * as_rec_get_string(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	return as_string_fromval(v);
}

/**
 *	Get a bin's value as an as_bytes.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline as_bytes * as_rec_get_bytes(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	return as_bytes_fromval(v);
}

/**
 *	Get a bin's value as an as_list.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline as_list * as_rec_get_list(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	return as_list_fromval(v);
}

/**
 *	Get a bin's value as an as_map.
 *
 *	@param rec		The as_rec to read the bin value from.
 *	@param name 	The name of the bin.
 *
 *	@return On success, the value of the bin. Otherwise NULL.
 */
inline as_map * as_rec_get_map(const as_rec * rec, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, rec, name);
	return as_map_fromval(v);
}

/******************************************************************************
 *	BIN SETTER FUNCTIONS
 ******************************************************************************/

/**
 *	Set the bin's value to an as_val.
 *
 *	@param rec 		The as_rec to write the bin value to - CONSUMES REFERENCE
 *	@param name 	The name of the bin.
 *	@param value 	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set(const as_rec * rec, const char * name, const as_val * value) 
{
	return as_util_hook(set, 1, rec, name, value);
}

/**
 *	Set the bin's value to an int64_t.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_int64(const as_rec * rec, const char * name, int64_t value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) as_integer_new(value));
}

/**
 *	Set the bin's value to a NULL terminated string.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_str(const as_rec * rec, const char * name, const char * value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) as_string_new(strdup(value), true));
}

/**
 *	Set the bin's value to an as_integer.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_integer(const as_rec * rec, const char * name, const as_integer * value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) value);
}

/**
 *	Set the bin's value to an as_string.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_string(const as_rec * rec, const char * name, const as_string * value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) value);
}

/**
 *	Set the bin's value to an as_bytes.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_bytes(const as_rec * rec, const char * name, const as_bytes * value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) value);
}

/**
 *	Set the bin's value to an as_list.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_list(const as_rec * rec, const char * name, const as_list * value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) value);
}

/**
 *	Set the bin's value to an as_map.
 *
 *	@param rec		The as_rec storing the bin.
 *	@param name 	The name of the bin.
 *	@param value	The value of the bin.
 *
 *	@return On success, 0. Otherwise an error occurred.
 */
inline int as_rec_set_map(const as_rec * rec, const char * name, const as_map * value) 
{
	return as_util_hook(set, 1, rec, name, (as_val *) value);
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 ******************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_rec_toval(const as_rec * rec) 
{
	return (as_val *) rec;
}

/**
 *	Convert from an as_val.
 */
inline as_rec * as_rec_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_REC, as_rec);
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_rec_val_destroy(as_val *);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_rec_val_hashcode(const as_val *v);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_rec_val_tostring(const as_val *v);
