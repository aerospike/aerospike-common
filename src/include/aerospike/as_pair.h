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
 *	MACROS
 ******************************************************************************/

#define pair_new(a,b) as_pair_new((as_val *) a, (as_val *) b)

/******************************************************************************
 *	TYPES
 ******************************************************************************/

/**
 *	A Pair of values: (_1,_2)
 */
typedef struct as_pair_s {

	/**
	 *	@private
	 *	as_boolean is a subtype of as_val.
	 *	You can cast as_boolean to as_val.
	 */
	as_val _;

	/**
	 *	The first value of the pair.
	 */
	as_val * _1;

	/**
	 *	The second value of the pair.
	 */
	as_val * _2;

} as_pair;

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Create and initializes a new heap allocated `as_pair`.
 *
 *	@param _1	The first value.
 *	@param _2	The second value.
 *
 *	@return On success, the new pair. Otherwise NULL.
 */
as_pair * as_pair_new(as_val * _1, as_val * _2);

/**
 *	Initializes a stack allocated `as_pair`.
 *
 *	@param p	The pair to initialize.
 *	@param _1	The first value.
 *	@param _2	The second value.
 *
 *	@return On success, the new pair. Otherwise NULL.
 */
as_pair * as_pair_init(as_pair * p, as_val * _1, as_val * _2);

/**
 *	Destroy the `as_pair` and release associated resources.
 */
inline void as_pair_destroy(as_pair * p)
{
	as_val_val_destroy((as_val *)p);
}

/******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

/**
 *	Get the first value of the pair
 */
inline as_val * as_pair_1(as_pair * p) 
{
	return p->_1;
}

/**
 *	Get the second value of the pair
 */
inline as_val * as_pair_2(as_pair * p) 
{
	return p->_2;
}

/******************************************************************************
 *	CONVERSION FUNCTIONS
 *****************************************************************************/

/**
 *	Convert to an as_val.
 */
inline as_val * as_pair_toval(const as_pair * p) 
{
	return (as_val *)p;
}

/**
 *	Convert from an as_val.
 */
inline as_pair * as_pair_fromval(const as_val * v) 
{
	return as_util_fromval(v, AS_PAIR, as_pair);
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Internal helper function for destroying an as_val.
 */
void as_pair_val_destroy(as_val *);

/**
 *	@private
 *	Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_pair_val_hashcode(const as_val *);

/**
 *	@private
 *	Internal helper function for getting the string representation of an as_val.
 */
char * as_pair_val_tostring(const as_val *);
