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

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

/**
 * Boolean type as_val.
 */
struct as_boolean_s {
	as_val  _;
	bool    value;
};

typedef struct as_boolean_s as_boolean;

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/**
 * Global TRUE value
 */
extern const as_boolean as_true;

/**
 * Global FALSE value
 */
extern const as_boolean as_false;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Initialize a stack allocated as_boolean.
 */
as_boolean * as_boolean_init(as_boolean * v, bool b);

/**
 * Creates a new heap allocated as_boolean.
 */
as_boolean * as_boolean_new(bool b);

/**
 * PRIVATE:
 * Internal helper function for destroying an as_val.
 */
void as_boolean_val_destroy(as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_boolean_val_hashcode(const as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the string representation of an as_val.
 */
char * as_boolean_val_tostring(const as_val *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Destroy the as_boolean and associated resources.
 */
inline void as_boolean_destroy(as_boolean * b) {
	as_val_val_destroy( (as_val *) b );
}

/**
 * Get the bool value.
 */
inline bool as_boolean_tobool(const as_boolean * b) {
	return b->value;
}

/******************************************************************************
 * CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 * Convert to an as_val.
 */
inline as_val * as_boolean_toval(const as_boolean * b) {
	return (as_val *) b;
}

/**
 * Convert from an as_val.
 */
inline as_boolean * as_boolean_fromval(const as_val * v) {
	return as_util_fromval(v, AS_BOOLEAN, as_boolean);
}
