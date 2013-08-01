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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <citrusleaf/cf_alloc.h>
#include <aerospike/as_boolean.h>

/******************************************************************************
 *	CONSTANTS
 *****************************************************************************/

const as_boolean as_true = {
	._ = { 
		.type = AS_BOOLEAN, 
		.free = false, 
		.count = 0
	},
	.value = true
};

const as_boolean as_false = {
	._.type = AS_BOOLEAN,
	._.free = false,
	._.count = 0,
	.value = false
};

/******************************************************************************
 *	INLINE FUNCTIONS
 ******************************************************************************/
 
extern inline void          as_boolean_destroy(as_boolean * boolean);

extern inline bool          as_boolean_getorelse(const as_boolean * boolean, bool fallback);
extern inline bool          as_boolean_get(const as_boolean * boolean);
extern inline bool          as_boolean_tobool(const as_boolean * boolean);

extern inline as_val *      as_boolean_toval(const as_boolean * boolean);
extern inline as_boolean *  as_boolean_fromval(const as_val * v);

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

static inline as_boolean * as_boolean_cons(as_boolean * boolean, bool free, bool value)
{
	if ( !boolean ) return boolean;

	as_val_cons((as_val *) boolean, AS_BOOLEAN, free);
	boolean->value = value;
	return boolean;
}

as_boolean * as_boolean_init(as_boolean * boolean, bool value)
{
	return as_boolean_cons(boolean, false, value);
}

as_boolean * as_boolean_new(bool value)
{
	as_boolean * boolean = (as_boolean *) malloc(sizeof(as_boolean));
	return as_boolean_cons(boolean, true, value);
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

void as_boolean_val_destroy(as_val * v)
{
	as_boolean * boolean = as_boolean_fromval(v);
	if ( !boolean ) return;

	boolean->value = false;
}

uint32_t as_boolean_val_hashcode(const as_val * v)
{
	as_boolean * boolean = as_boolean_fromval(v);
	return boolean != NULL && boolean->value ? 1 : 0;
}

char * as_boolean_val_tostring(const as_val * v)
{
	if ( as_val_type(v) != AS_BOOLEAN ) return NULL;

	as_boolean * b = (as_boolean *) v;
	char * str = (char *) malloc(sizeof(char) * 6);
	bzero(str,6);
	if ( b->value ) {
		strcpy(str,"true");
	}
	else {
		strcpy(str,"false");
	}
	return str;

}
