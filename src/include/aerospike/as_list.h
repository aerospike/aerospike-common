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

#include <stdbool.h>
#include <inttypes.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_string.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_iterator.h>

#include <aerospike/as_linkedlist.h>
#include <aerospike/as_arraylist.h>


/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_map_s;

struct as_list_hooks_s;

/**
 * Callback function for as_list_foreach()
 */
typedef bool (* as_list_foreach_callback) (as_val *, void *);


/**
 * List Data
 */
union as_list_data_u {
	as_arraylist    arraylist;
	as_linkedlist   linkedlist;
	void *          generic;
};

typedef union as_list_data_u as_list_data;

/**
 * List Object
 */
struct as_list_s {
	as_val                          _;
	as_list_data                    data;
	const struct as_list_hooks_s *  hooks;
};

typedef struct as_list_s as_list;

/**
 * List Function Hooks
 */
struct as_list_hooks_s {
	bool            (* destroy)(as_list *);
	uint32_t        (* hashcode)(const as_list *);

	uint32_t        (* size)(const as_list *);
	int             (* append)(as_list *, as_val *);
	int             (* prepend)(as_list *, as_val *);
	as_val *        (* get)(const as_list *, const uint32_t);
	int             (* set)(as_list *, const uint32_t, as_val *);
	as_val *        (* head)(const as_list *);
	as_list *       (* tail)(const as_list *);
	as_list *       (* drop)(const as_list *, uint32_t);
	as_list *       (* take)(const as_list *, uint32_t);
	
	bool            (* foreach)(const as_list *, as_list_foreach_callback, void *);
	as_iterator *   (* iterator_init)(const as_list *, as_iterator *);
	as_iterator *   (* iterator_new)(const as_list *);
};

typedef struct as_list_hooks_s as_list_hooks;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Initializes a stack allocated as_list.
 */
as_list * as_list_init(as_list *, void *, const as_list_hooks *);

/**
 * Creates a new heap allocated as_list.
 */
as_list * as_list_new(void *, const as_list_hooks *);

/**
 * PRIVATE:
 * Internal helper function for destroying an as_val.
 */
void as_list_val_destroy(as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_list_val_hashcode(const as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the string representation of an as_val.
 */
char * as_list_val_tostring(const as_val * v);

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

/**
 * Destroy the list and associated resources.
 */
inline void as_list_destroy(as_list * l) 
{
	as_val_val_destroy((as_val *) l);
}

/**
 * Number of elements in the list.
 */
inline uint32_t as_list_size(as_list * l) 
{
	return as_util_hook(size, 0, l);
}

/**
 * The first element in the list.
 */
inline as_val * as_list_head(const as_list * l) 
{
	return as_util_hook(head, NULL, l);
}

/**
 * All elements after the first element in the list.
 */
inline as_list * as_list_tail(const as_list * l) 
{
	return as_util_hook(tail, NULL, l);
}

/**
 * Create a new list containing all elements, except the first n elements, of the list.
 */
inline as_list * as_list_drop(const as_list * l, uint32_t n) 
{
	return as_util_hook(drop, NULL, l, n);
}

/**
 * Creates a new list containing the first n elements of the list.
 */
inline as_list * as_list_take(const as_list * l, uint32_t n) 
{
	return as_util_hook(take, NULL, l, n);
}

/******************************************************************************
 * GETTER FUNCTIONS
 *****************************************************************************/

/**
 * Get the value at specified index as an as_val.
 */
inline as_val * as_list_get(const as_list * l, const uint32_t i) 
{
	return as_util_hook(get, NULL, l, i);
}

/**
 * Get the value at specified index as an int64_t.
 */
inline int64_t as_list_get_int64(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	as_integer * iv = as_integer_fromval(v);
	return iv ? as_integer_toint(iv) : 0;
}

/**
 * Get the value at specified index as an NULL terminated string.
 */
inline char * as_list_get_str(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	as_string * sv = as_string_fromval(v);
	return sv ? as_string_tostring(sv) : NULL;
}

/**
 * Get the value at specified index as an as_integer.
 */
inline as_integer * as_list_get_integer(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	return v && v->type == AS_INTEGER ? (as_integer *) v : NULL;
}

/**
 * Get the value at specified index as an as_val.
 */
inline as_string * as_list_get_string(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	return v && v->type == AS_STRING ? (as_string *) v : NULL;
}

/**
 * Get the value at specified index as an as_val.
 */
inline as_bytes * as_list_get_bytes(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	return v && v->type == AS_BYTES ? (as_bytes *) v : NULL;
}

/**
 * Get the value at specified index as an as_val.
 */
inline as_list * as_list_get_list(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	return v && v->type == AS_LIST ? (as_list *) v : NULL;
}

/**
 * Get the value at specified index as an as_val.
 */
inline struct as_map_s * as_list_get_map(const as_list * l, const uint32_t i) 
{
	as_val * v = as_util_hook(get, NULL, l, i);
	return v && v->type == AS_MAP ? (struct as_map_s *) v : NULL;
}


/******************************************************************************
 * SETTER FUNCTIONS
 *****************************************************************************/

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set(as_list * l, const uint32_t i, as_val * value) 
{
	return as_util_hook(set, 1, l, i, value);
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_int64(as_list * l, const uint32_t i, int64_t value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) as_integer_new(i));
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_str(as_list * l, const uint32_t i, const char * value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) as_string_new(strdup(value), true));
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_integer(as_list * l, const uint32_t i, as_integer * value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) value);
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_string(as_list * l, const uint32_t i, as_string * value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) value);
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_bytes(as_list * l, const uint32_t i, as_bytes * value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) value);
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_list(as_list * l, const uint32_t i, as_list * value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) value);
}

/**
 * Set the value at specified index as an as_val.
 */
inline int as_list_set_map(as_list * l, const uint32_t i, struct as_map_s * value) 
{
	return as_util_hook(set, 1, l, i, (as_val *) value);
}

/******************************************************************************
 * APPEND FUNCTIONS
 *****************************************************************************/

/**
 * Append an as_val to the list
 */
inline int as_list_append(as_list * l, as_val * value) 
{
	return as_util_hook(append, 1, l, value);
}

/**
 * Append an int64_t to the list
 */
inline int as_list_append_int64(as_list * l, int64_t value) 
{
	return as_util_hook(append, 1, l, (as_val *) as_integer_new(value));
}

/**
 * Append a NULL terminates string to the list
 */
inline int as_list_append_str(as_list * l, const char * value) 
{
	return as_util_hook(append, 1, l, (as_val *) as_string_new(strdup(value), true));
}

/**
 * Append an as_integer to the list
 */
inline int as_list_append_integer(as_list * l, as_integer * value) 
{
	return as_util_hook(append, 1, l, (as_val *) value);
}

/**
 * Append an as_string to the list
 */
inline int as_list_append_string(as_list * l, as_string * value) 
{
	return as_util_hook(append, 1, l, (as_val *) value);
}

/**
 * Append an as_bytes to the list
 */
inline int as_list_append_bytes(as_list * l, as_bytes * value) 
{
	return as_util_hook(append, 1, l, (as_val *) value);
}

/**
 * Append an as_list to the list
 */
inline int as_list_append_list(as_list * l, as_list * value) 
{
	return as_util_hook(append, 1, l, (as_val *) value);
}

/**
 * Append an as_map to the list
 */
inline int as_list_append_map(as_list * l, struct as_map_s * value) 
{
	return as_util_hook(append, 1, l, (as_val *) value);
}

/******************************************************************************
 * PREPEND FUNCTIONS
 *****************************************************************************/

/**
 * Prepend an as_val to the list
 */
inline int as_list_prepend(as_list * l, as_val * v) 
{
	return as_util_hook(prepend, 1, l, v);
}

/**
 * Prepend an int64_t to the list
 */
inline int as_list_prepend_int64(as_list * l, int64_t value) 
{
	return as_util_hook(prepend, 1, l, (as_val *) as_integer_new(value));
}

/**
 * Prepend a NULL terminates string to the list
 */
inline int as_list_prepend_str(as_list * l, const char * value) 
{
	return as_util_hook(prepend, 1, l, (as_val *) as_string_new(strdup(value), true));
}

/**
 * Prepend an as_integer to the list
 */
inline int as_list_prepend_integer(as_list * l, as_integer * value) 
{
	return as_util_hook(prepend, 1, l, (as_val *) value);
}

/**
 * Prepend an as_string to the list
 */
inline int as_list_prepend_string(as_list * l, as_string * value) 
{
	return as_util_hook(prepend, 1, l, (as_val *) value);
}

/**
 * Prepend an as_bytes to the list
 */
inline int as_list_prepend_bytes(as_list * l, as_bytes * value)
{
	return as_util_hook(prepend, 1, l, (as_val *) value);
}

/**
 * Prepend an as_list to the list
 */
inline int as_list_prepend_list(as_list * l, as_list * value) 
{
	return as_util_hook(prepend, 1, l, (as_val *) value);
}

/**
 * Prepend an as_map to the list
 */
inline int as_list_prepend_map(as_list * l, struct as_map_s * value) 
{
	return as_util_hook(prepend, 1, l, (as_val *) value);
}

/******************************************************************************
 * ITERATION FUNCTIONS
 *****************************************************************************/

inline void as_list_foreach(const as_list * l, as_list_foreach_callback callback, void * udata) 
{
	as_util_hook(foreach, false, l, callback, udata);
}

inline as_iterator * as_list_iterator_init(as_iterator * i, const as_list * l) 
{
	return as_util_hook(iterator_init, NULL, l, i);
}

inline as_iterator * as_list_iterator_new(const as_list * l) 
{
	return as_util_hook(iterator_new, NULL, l);
}

/******************************************************************************
 * CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 * Convert to an as_val.
 */
inline as_val * as_list_toval(as_list * l) 
{
	return (as_val *) l;
}

/**
 * Convert from an as_val.
 */
inline as_list * as_list_fromval(as_val * v) 
{
	return as_util_fromval(v, AS_LIST, as_list);
}

