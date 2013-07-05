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

/**
 *	as_stringmap provides a convience interface for populating a map with
 *	string keys.
 *
 *	@addtogroup stringmap_t StringMap
 *	@{
 */

#pragma once

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/******************************************************************************
 *	SETTER FUNCTIONS
 *****************************************************************************/

/**
 *	Set the specified key's value to an as_val.
 */
inline int as_stringmap_set(as_map * m, const char * k, as_val * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), v);
}

/**
 *	Set the specified key's value to an int64_t.
 */
inline int as_stringmap_set_int64(as_map * m, const char * k, int64_t v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) as_integer_new(v));
}

/**
 *	Set the specified key's value to a NULL terminated string.
 */
inline int as_stringmap_set_str(as_map * m, const char * k, const char * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) as_string_new(strdup(v),true));
}

/**
 *	Set the specified key's value to an as_integer.
 */
inline int as_stringmap_set_integer(as_map * m, const char * k, as_integer * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) v);
}

/**
 *	Set the specified key's value to an as_string.
 */
inline int as_stringmap_set_string(as_map * m, const char * k, as_string * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) v);
}

/**
 *	Set the specified key's value to an as_bytes.
 */
inline int as_stringmap_set_bytes(as_map * m, const char * k, as_bytes * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) v);
}

/**
 *	Set the specified key's value to an as_list.
 */
inline int as_stringmap_set_list(as_map * m, const char * k, as_list * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) v);
}

/**
 *	Set the specified key's value to an as_map.
 */
inline int as_stringmap_set_map(as_map * m, const char * k, as_map * v) 
{
	return as_util_hook(set, 1, m, (as_val *) as_string_new(strdup(k),true), (as_val *) v);
}

/******************************************************************************
 *	GETTER FUNCTIONS
 *****************************************************************************/

/**
 *	Get the specified key's value as an as_val.
 */
inline as_val * as_stringmap_get(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	return v;
}

/**
 *	Get the specified key's value as an int64_t.
 */
inline int64_t as_stringmap_get_int64(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	as_integer * i = as_integer_fromval(v);
	return i ? as_integer_toint(i) : 0;
}

/**
 *	Get the specified key's value as a NULL terminated string.
 */
inline char * as_stringmap_get_str(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	as_string * s = as_string_fromval(v);
	return s ? as_string_tostring(s) : NULL;
}

/**
 *	Get the specified key's value as an as_integer.
 */
inline as_integer * as_stringmap_get_integer(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	return as_integer_fromval(v);
}

/**
 *	Get the specified key's value as an as_string.
 */
inline as_string * as_stringmap_get_string(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	return as_string_fromval(v);
}

/**
 *	Get the specified key's value as an as_bytes.
 */
inline as_bytes * as_stringmap_get_bytes(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	return as_bytes_fromval(v);
}

/**
 *	Get the specified key's value as an as_list.
 */
inline as_list * as_stringmap_get_list(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	return as_list_fromval(v);
}

/**
 *	Get the specified key's value as an as_map.
 */
inline as_map * as_stringmap_get_map(as_map * m, const char * k) 
{
	as_string key;
	as_val * v = as_util_hook(get, NULL, m, (as_val *) as_string_init(&key, (char *) k, false));
	return as_map_fromval(v);
}

/**
 *	@}
 */
