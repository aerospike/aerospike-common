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

#include <citrusleaf/cf_atomic.h>

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

/**
 *	as_val types
 */
typedef enum as_val_t {
	AS_UNKNOWN      = 0,
	AS_NIL          = 1,
	AS_BOOLEAN      = 2,
	AS_INTEGER      = 3,
	AS_STRING       = 4,
	AS_LIST         = 5,
	AS_MAP          = 6,
	AS_REC          = 7,
	AS_PAIR         = 8,
	AS_BYTES        = 9
} __attribute__((packed)) as_val_t;

/**
 *	Represents a value
 */
typedef struct as_val_s {

	/**
	 *	Value type
	 */
	enum as_val_t type;

	/**
	 *	Value can be freed.
	 *	Should be false for stack allocated values.
	 */
	bool free;

	/**
	 *	Reference count
	 *	Values are ref counted.
	 */
	cf_atomic32 count;

} as_val;

/******************************************************************************
 *	MACROS
 *****************************************************************************/

/**
 *	Get the type of the specified value.
 */
#define as_val_type(__v) (((as_val *)__v)->type)

/**
 *	Increase the refcount of the value.
 */
#define as_val_reserve(__v) ( as_val_val_reserve((as_val *)__v) )

/**
 *	Decrement the refcount of the value. When it reaches 0 (zero) and 
 *	`free` is true, then free the value.
 */
#define as_val_destroy(__v) ( as_val_val_destroy((as_val *)__v) )

/**
 *	Get the hashcode of the value.
 */
#define as_val_hashcode(__v) ( as_val_val_hashcode((as_val *)__v) )

/**
 *	Get the string representation of the value.
 */
#define as_val_tostring(__v) ( as_val_val_tostring((as_val *)__v) )

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Helper function for incrementing the count of a value.
 */
as_val * as_val_val_reserve(as_val *);

/**
 *	@private
 *	Helper function for decrementing the count of a value,
 *	and if count==0 and free==true, then free the value.
 */
as_val * as_val_val_destroy(as_val *);

/**
 *	@private
 *	Helper function for calculating the hash value.
 */
uint32_t as_val_val_hashcode(const as_val *);

/**
 *	@private
 *	Helper function for generating the string representation.
 */
char * as_val_val_tostring(const as_val *);

/******************************************************************************
 *	INLINE FUNCTIONS
 *****************************************************************************/

/**
 *	Initializes as_val types.
 */
inline void as_val_init(as_val * v, as_val_t type, bool free) 
{
	v->type = type; 
	v->free = free; 
	v->count = 1;
}

