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
 ******************************************************************************/

typedef struct as_string_s as_string;

/**
 * String value
 */
struct as_string_s {
	as_val      _;
	bool        free; 
	char *      value;
	size_t      len;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Initialize a stack allocated as_string.
 *
 * If free is true, then the string value will be freed when the as_string is destroyed.
 *
 * @param s 		- the stack allocated as_string to initialize
 * @param value 	- the NULL terminated string of character.
 * @param free 		- if true, then the value will be freed when as_string is destroyed.
 *
 * @return the initialized as_string on success, otherwise NULL.
 */
as_string * as_string_init(as_string * s, char * value, bool free);

/**
 * Creates a new heap allocated as_string.
 *
 * If free is true, then the string value will be freed when the as_string is destroyed.
 *
 * @param s 		- the stack allocated as_string to initialize
 * @param value 	- the NULL terminated string of character.
 * @param free 		- if true, then the value will be freed when as_string is destroyed.
 *
 * @return the newly allocated as_string on success, otherwise NULL.
 */
as_string * as_string_new(char * value, bool free);

/**
 * The length of the string
 *
 * @param s - the string to get the length of. 
 *
 * @return the length of the string in bytes.
 */
size_t as_string_len(as_string * s);

/**
 * PRIVATE:
 * Internal helper function for destroying an as_val.
 */
void as_string_val_destroy(as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_string_val_hashcode(const as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the string representation of an as_val.
 */
char * as_string_val_tostring(const as_val * v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Destroy the as_string and associated resources.
 */
inline void as_string_destroy(as_string * s) 
{
	as_val_val_destroy((as_val *) s);
}

/**
 * Get the string value.
 */
inline char * as_string_tostring(const as_string * s) 
{
	return s ? s->value : NULL;
}

/******************************************************************************
 * CONVERSION FUNCTIONS
 ******************************************************************************/

/**
 * Convert to an as_val.
 */
inline as_val * as_string_toval(const as_string * s) 
{
	return (as_val *) s;
}

/**
 * Convert from an as_val.
 */
inline as_string * as_string_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_STRING, as_string);
}
