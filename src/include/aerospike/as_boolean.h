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

/******************************************************************************
 *	TYPES
 ******************************************************************************/

/**
 *	Boolean value.
 *
 *	To use the boolean value, you should use one of the two constants:
 *
 *		as_boolean as_true;
 *		as_boolean as_false;
 *	
 *	Both `as_boolean_init()` and `as_boolean_new()` should be used sparingly.
 *
 *	@extends as_val
 *	@ingroup aerospike_t
 */
typedef struct as_boolean_s {

	/**
	 *	@private
	 *	as_boolean is a subtype of as_val.
	 *	You can cast as_boolean to as_val.
	 */
	as_val _;

	/**
	 *	The boolean value.
	 */
	bool value;

} as_boolean;

/******************************************************************************
 *	CONSTANTS
 *****************************************************************************/

/**
 *	True value.
 *
 *	Use this when you need to use an `as_boolean` containing `true`,
 *	rather than allocating a new `as_boolean`.
 */
extern const as_boolean as_true;

/**
 *	False value.
 *
 *	Use this when you need to use an `as_boolean` containing `true`,
 *	rather than allocating a new `as_boolean`.
 */
extern const as_boolean as_false;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated `as_boolean` with the given boolean value.
 *
 *	@param boolean	The `as_boolean` to initialize.
 *	@param value	The bool value.
 *
 *	@return On succes, the initialized value. Otherwise NULL.
 *
 *	@relatesalso as_boolean
 */
as_boolean * as_boolean_init(as_boolean * boolean, bool value);

/**
 *	Creates a new heap allocated `as_boolean` and initializes with
 *	the given boolean value.
 *
 *	@param value	The bool value.
 *
 *	@return On succes, the newly allocated value. Otherwise NULL.
 *
 *	@relatesalso as_boolean
 */
as_boolean * as_boolean_new(bool value);

/**
 *	Destroy the `as_boolean` and release associated resources.
 *
 *	@param boolean 	The `as_boolean` to destroy.
 *
 *	@relatesalso as_boolean
 */
inline void as_boolean_destroy(as_boolean * boolean) {
	as_val_destroy((as_val *) boolean);
}

/******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

/**
 *	Get the bool value. If boolean is NULL, then return the fallback value.
 *
 *	@relatesalso as_boolean
 */
inline bool as_boolean_getorelse(const as_boolean * boolean, bool fallback) {
	return boolean ? boolean->value : fallback;
}

/**
 *	Get the bool value.
 *
 *	@relatesalso as_boolean
 */
inline bool as_boolean_get(const as_boolean * boolean) {
	return as_boolean_getorelse(boolean, false);
}

/**
 *	Get the bool value.
 *	@deprecated Use as_boolean_get() instead.
 *
 *	@relatesalso as_boolean
 */
inline bool as_boolean_tobool(const as_boolean * boolean) {
	return as_boolean_getorelse(boolean, false);
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 *	Convert to an as_val.
 *
 *	@relatesalso as_boolean
 */
inline as_val * as_boolean_toval(const as_boolean * boolean) {
	return (as_val *) boolean;
}

/**
 *	Convert from an as_val.
 *
 *	@relatesalso as_boolean
 */
inline as_boolean * as_boolean_fromval(const as_val * v) {
	return as_util_fromval(v, AS_BOOLEAN, as_boolean);
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_boolean_val_destroy(as_val * v);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_boolean_val_hashcode(const as_val * v);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_boolean_val_tostring(const as_val * v);
