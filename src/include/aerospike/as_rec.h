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

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_rec_hooks_s;

/**
 * Record Structure
 * Contains a pointer to the source of the record and 
 * hooks that interface with the source.
 *
 * @field source contains record specific data.
 * @field hooks contains the record interface that works with the source.
 */
struct as_rec_s {
	as_val                          _;
	void *                          data;
	const struct as_rec_hooks_s *   hooks;
};

typedef struct as_rec_s as_rec;

/**
 * Record Interface.
 * Provided functions that interface with the records.
 */
struct as_rec_hooks_s {
	bool        (* destroy)(as_rec *);
	uint32_t    (* hashcode)(const as_rec *);

	as_val *    (* get)(const as_rec *, const char *);
	int         (* set)(const as_rec *, const char *, const as_val *);
	int         (* remove)(const as_rec *, const char *);
	uint32_t    (* ttl)(const as_rec *);
	uint16_t    (* gen)(const as_rec *);
	uint16_t    (* numbins)(const as_rec *);
	as_bytes *  (* digest)(const as_rec *);
	int         (* set_flags)(const as_rec*, const char *, uint8_t );
	int         (* set_type)(const as_rec*,  uint8_t );
};

typedef struct as_rec_hooks_s as_rec_hooks;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize an as_rec allocated on the stack.
 */
as_rec *  as_rec_init(as_rec *, void *, const as_rec_hooks *);

/**
 * Creates a new as_rec allocated on the heap.
 */
as_rec *  as_rec_new(void *, const as_rec_hooks *);

/**
 * PRIVATE:
 * Internal helper function for destroying an as_val.
 */
void as_rec_val_destroy(as_val *);

/**
 * PRIVATE:
 * Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_rec_val_hashcode(const as_val *v);

/**
 * PRIVATE:
 * Internal helper function for getting the string representation of an as_val.
 */
char * as_rec_val_tostring(const as_val *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Get the data source for the record.
 */
inline void * as_rec_source(const as_rec * r) 
{
	return r ? r->data : NULL;
}

/**
 * Destroy the record.
 */
inline void as_rec_destroy(as_rec *r) 
{
	as_val_val_destroy((as_val *) r);
}

/**
 * Remove a bin from a record.
 *
 * @param r 	- the record to remove the bin from.
 * @param name 	- the name of the bin to remove.
 *
 * @return 0 on success, otherwise an error occurred.
 */
inline int as_rec_remove(const as_rec * r, const char * name) 
{
	return as_util_hook(remove, 1, r, name);
}

/**
 * Get the ttl for the record.
 */
inline uint32_t as_rec_ttl(const as_rec * r) 
{
	return as_util_hook(ttl, 0, r);
}

/**
 * Get the generation of the record
 */
inline uint16_t as_rec_gen(const as_rec * r) 
{
	return as_util_hook(gen, 0, r);
}

/**
 * Get the number of bins in the record.
 */
inline uint16_t as_rec_numbins(const as_rec * r) 
{
	return as_util_hook(numbins, 0, r);
}

/**
 * Get the digest of the record.
 */
inline as_bytes * as_rec_digest(const as_rec * r) 
{
	return as_util_hook(digest, 0, r);
}

/**
 * Set flags on a bin.
 */
inline int  as_rec_set_flags(const as_rec * r, const char * name, uint8_t flags) 
{
	return as_util_hook(set_flags, 0, r, name, flags);
}

/**
 * Set the record type.
 */
inline int  as_rec_set_type(const as_rec * r, uint8_t rec_type) 
{
	return as_util_hook(set_type, 0, r, rec_type);
}

/******************************************************************************
 * BIN GETTER FUNCTIONS
 ******************************************************************************/

/**
 * Get a bin's value.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline as_val * as_rec_get(const as_rec * r, const char * name) 
{
	return as_util_hook(get, NULL, r, name);
}

/**
 * Get a bin's value as an int64_t.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise 0.
 */
inline int64_t as_rec_get_int64(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	as_integer * i = as_integer_fromval(v);
	return i ? as_integer_toint(i) : 0;
}

/**
 * Get a bin's value as a NULL terminated string.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline char * as_rec_get_str(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	as_string * s = as_string_fromval(v);
	return s ? as_string_tostring(s) : 0;
}

/**
 * Get a bin's value as an as_integer.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline as_integer * as_rec_get_integer(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	return as_integer_fromval(v);
}

/**
 * Get a bin's value as an as_string.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline as_string * as_rec_get_string(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	return as_string_fromval(v);
}

/**
 * Get a bin's value as an as_bytes.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline as_bytes * as_rec_get_bytes(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	return as_bytes_fromval(v);
}

/**
 * Get a bin's value as an as_list.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline as_list * as_rec_get_list(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	return as_list_fromval(v);
}

/**
 * Get a bin's value as an as_map.
 *
 * @param r 	- the as_rec to read the bin value from.
 * @param name 	- the name of the bin.
 *
 * @return the value of the bin if successful. Otherwise NULL.
 */
inline as_map * as_rec_get_map(const as_rec * r, const char * name) 
{
	as_val * v = as_util_hook(get, NULL, r, name);
	return as_map_fromval(v);
}

/******************************************************************************
 * BIN SETTER FUNCTIONS
 ******************************************************************************/

/**
 * Set the bin's value to an as_val.
 *
 * @param r the as_rec to write the bin value to - CONSUMES REFERENCE
 * @param name the name of the bin.
 * @param value the value of the bin.
 */
inline int as_rec_set(const as_rec * r, const char * name, const as_val * value) 
{
	return as_util_hook(set, 1, r, name, value);
}

/**
 * Set the bin's value to an int64_t.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_int64(const as_rec * r, const char * name, int64_t value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) as_integer_new(value));
}

/**
 * Set the bin's value to a NULL terminated string.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_str(const as_rec * r, const char * name, const char * value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) as_string_new(strdup(value), true));
}

/**
 * Set the bin's value to an as_integer.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_integer(const as_rec * r, const char * name, const as_integer * value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) value);
}

/**
 * Set the bin's value to an as_string.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_string(const as_rec * r, const char * name, const as_string * value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) value);
}

/**
 * Set the bin's value to an as_bytes.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_bytes(const as_rec * r, const char * name, const as_bytes * value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) value);
}

/**
 * Set the bin's value to an as_list.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_list(const as_rec * r, const char * name, const as_list * value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) value);
}

/**
 * Set the bin's value to an as_map.
 *
 * @param r 	- the as_rec storing the bin.
 * @param name 	- the name of the bin.
 * @param value	- the value of the bin.
 */
inline int as_rec_set_map(const as_rec * r, const char * name, const as_map * value) 
{
	return as_util_hook(set, 1, r, name, (as_val *) value);
}

/******************************************************************************
 * CONVERSION FUNCTIONS
 ******************************************************************************/

/**
 * Convert to an as_val.
 */
inline as_val * as_rec_toval(const as_rec * r) 
{
	return (as_val *) r;
}

/**
 * Convert from an as_val.
 */
inline as_rec * as_rec_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_REC, as_rec);
}
