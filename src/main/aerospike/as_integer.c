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
#include <aerospike/as_integer.h>

/******************************************************************************
 *	INLINE FUNCTIONS
 ******************************************************************************/

extern inline void			as_integer_destroy(as_integer * integer);

extern inline int64_t		as_integer_getorelse(const as_integer * integer, int64_t fallback);
extern inline int64_t		as_integer_get(const as_integer * integer);
extern inline int64_t		as_integer_toint(const as_integer * integer);

extern inline as_val *		as_integer_toval(const as_integer * integer);
extern inline as_integer *	as_integer_fromval(const as_val * v);

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

static as_integer * as_integer_cons(as_integer * integer, bool free, int64_t value)
{
	if ( !integer ) return integer;

	as_val_cons((as_val *) integer, AS_INTEGER, free);
	integer->value = value;
	return integer;
}

as_integer * as_integer_init(as_integer * integer, int64_t value)
{
	return as_integer_cons(integer, false, value);
}

as_integer * as_integer_new(int64_t value)
{
	as_integer * integer = (as_integer *) malloc(sizeof(as_integer));
	return as_integer_cons(integer, true, value);
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

void as_integer_val_destroy(as_val * v)
{
	as_integer * integer = as_integer_fromval(v);
	if ( !integer ) return;
	
	integer->value = INT64_MIN;
}

uint32_t as_integer_val_hashcode(const as_val * v)
{
	as_integer * i = as_integer_fromval(v);
	return i != NULL ? i->value : 0;
}

char * as_integer_val_tostring(const as_val * v)
{
	as_integer * i = (as_integer *) v;
	char * str = (char *) malloc(sizeof(char) * 32);
	bzero(str, 32);
	sprintf(str,"%lu",i->value);
	return str;
}
