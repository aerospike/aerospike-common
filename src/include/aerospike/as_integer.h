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

#include <stdint.h>

/******************************************************************************
 *	TYPES
 ******************************************************************************/

/**
 *	Integer value.
 *
 *	@extends as_val
 */
typedef struct as_integer_s {

	/**
	 *	@private
	 *	as_boolean is a subtype of as_val.
	 *	You can cast as_boolean to as_val.
	 */
	as_val  _;

	/**
	 *	The integer value
	 */
	int64_t value;

} as_integer;

/******************************************************************************
 *	FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated `as_integer` with the given integer value.
 *
 *	~~~~~~~~~~{.c}
 *	as_integer i;
 *	as_integer_init(&i, 123);
 *	~~~~~~~~~~
 *
 *	When the `as_integer` is no longer needed, you should release it an it's 
 *	resources:
 *
 *	~~~~~~~~~~{.c}
 *	as_integer_destroy(&i);
 *	~~~~~~~~~~
 *
 *	@param v		The `as_integer` to initialize.
 *	@param i		The integer value.
 *
 *	@return On succes, the initialized value. Otherwise NULL.
 */
as_integer * as_integer_init(as_integer * v, int64_t i);

/**
 *	Creates a new heap allocated as_integer.
 *
 *	~~~~~~~~~~{.c}
 *	as_integer * i = as_integer_new(123);
 *	~~~~~~~~~~
 *
 *	When the `as_integer` is no longer needed, you should release it an it's 
 *	resources:
 *
 *	~~~~~~~~~~{.c}
 *	as_integer_destroy(&i);
 *	~~~~~~~~~~
 *
 *	@param i		The integer value.
 *
 *	@return On succes, the initialized value. Otherwise NULL.
 */
as_integer * as_integer_new(int64_t i);

/**
 *	Destroy the `as_integer` and release resources.
 *
 *	~~~~~~~~~~{.c}
 *	as_integer_destroy(i);
 *	~~~~~~~~~~
 *
 *	@param i	The integer to destroy.
 */
inline void as_integer_destroy(as_integer * i) {
	as_val_val_destroy((as_val *) i);
}

/******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

/**
 *	Get the int64_t value.
 */
inline int64_t as_integer_toint(const as_integer * i) {
	return i ? i->value : 0;
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 ******************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_integer_toval(const as_integer * i) {
	return (as_val *) i;
}

/**
 *	Convert from an as_val.
 */
inline as_integer * as_integer_fromval(const as_val * v) {
	return as_util_fromval(v, AS_INTEGER, as_integer);
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_integer_val_destroy(as_val * v);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_integer_val_hashcode(const as_val * v);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_integer_val_tostring(const as_val * v);
