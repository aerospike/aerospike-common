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

#include <citrusleaf/alloc.h>

#include <aerospike/as_boolean.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>
#include <aerospike/as_nil.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_rec.h>
#include <aerospike/as_string.h>
#include <aerospike/as_val.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

typedef void		(* as_val_destroy_callback)(as_val * v);
typedef uint32_t	(* as_val_hashcode_callback)(const as_val * v);
typedef as_val *	(* as_val_reserve_callback)(as_val * v);
typedef char *	(* as_val_tostring_callback)(const as_val * v);

/******************************************************************************
 *	INLINE FUNCTIONS
 *****************************************************************************/

extern inline void as_val_init(as_val *v, as_val_t type, bool free);
extern inline as_val * as_val_cons(as_val * val, as_val_t type, bool free);

/******************************************************************************
 *	STATIC FUNCTIONS
 *****************************************************************************/

static void     as_val_destroy_noop(as_val *);
static uint32_t as_val_hashcode_noop(const as_val *);
static char *   as_val_tostring_noop(const as_val *);
static as_val * as_val_reserve_noop(as_val * v);
static as_val * as_val_reserve_count(as_val * v);

/******************************************************************************
 *	VARIABLES
 *****************************************************************************/

static const as_val_destroy_callback as_val_destroy_callbacks[] = {
	[AS_UNKNOWN]	= as_val_destroy_noop,
	[AS_NIL]		= as_nil_val_destroy,
	[AS_BOOLEAN]	= as_val_destroy_noop,
	[AS_INTEGER]	= as_integer_val_destroy,
	[AS_STRING]		= as_string_val_destroy,
	[AS_BYTES]		= as_bytes_val_destroy,
	[AS_LIST]		= as_list_val_destroy,
	[AS_MAP]		= as_map_val_destroy,
	[AS_REC]		= as_rec_val_destroy,
	[AS_PAIR]		= as_pair_val_destroy
};		

static const as_val_tostring_callback as_val_tostring_callbacks[] = {
	[AS_UNKNOWN]	= as_val_tostring_noop,
	[AS_NIL]		= as_nil_val_tostring,
	[AS_BOOLEAN]	= as_boolean_val_tostring,
	[AS_INTEGER]	= as_integer_val_tostring,
	[AS_STRING]		= as_string_val_tostring,
	[AS_BYTES]		= as_bytes_val_tostring,
	[AS_LIST]		= as_list_val_tostring,
	[AS_MAP]		= as_map_val_tostring,
	[AS_REC]		= as_rec_val_tostring,
	[AS_PAIR]		= as_pair_val_tostring
};

static const as_val_hashcode_callback as_val_hashcode_callbacks[] = {
	[AS_UNKNOWN]	= as_val_hashcode_noop,
	[AS_NIL]		= as_nil_val_hashcode,
	[AS_BOOLEAN]	= as_boolean_val_hashcode,
	[AS_INTEGER]	= as_integer_val_hashcode,
	[AS_STRING]		= as_string_val_hashcode,
	[AS_BYTES]		= as_bytes_val_hashcode,
	[AS_LIST]		= as_list_val_hashcode,
	[AS_MAP]		= as_map_val_hashcode,
	[AS_REC]		= as_rec_val_hashcode,
	[AS_PAIR]		= as_pair_val_hashcode
};

static const as_val_reserve_callback as_val_reserve_callbacks[] = {
	[AS_UNKNOWN]	= as_val_reserve_noop,
	[AS_NIL]		= as_val_reserve_noop,
	[AS_BOOLEAN]	= as_val_reserve_noop,
	[AS_INTEGER]	= as_val_reserve_count,
	[AS_STRING]		= as_val_reserve_count,
	[AS_BYTES]		= as_val_reserve_count,
	[AS_LIST]		= as_val_reserve_count,
	[AS_MAP]		= as_val_reserve_count,
	[AS_REC]		= as_val_reserve_count,
	[AS_PAIR]		= as_val_reserve_count
};

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

static void as_val_destroy_noop(as_val * v)
{
	return;
}

static as_val * as_val_reserve_noop(as_val * v)
{
	// return without ref-counting !!
	return v;
}

static as_val * as_val_reserve_count(as_val * v)
{
	// return after ref-counting !!
	cf_atomic32_add(&(v->count),1);
	return v;
}

static uint32_t as_val_hashcode_noop(const as_val * v)
{ 
	return 0;
}

static char * as_val_tostring_noop(const as_val * v)
{ 
	return 0;
}

as_val * as_val_val_reserve(as_val * v) 
{
	if ( !v ) return v;
	return as_val_reserve_callbacks[ v->type ](v);
	// cf_atomic32_add(&(v->count),1);
	// return v;
}

as_val * as_val_val_destroy(as_val * v)
{
	if ( v == NULL || !v->count ) return v;
	// if we reach the last reference, call the destructor, and free
	if ( 0 == cf_atomic32_decr(&(v->count)) ) {
		as_val_destroy_callbacks[ v->type ](v);     
		if ( v->free ) {
			cf_free(v);
		}
		v = NULL;
	}
	return v;
}

uint32_t as_val_val_hashcode(const as_val * v)
{
	if (v == 0) return 0;
	return as_val_hashcode_callbacks[ v->type ](v);
}

char * as_val_val_tostring(const as_val * v)
{
	if (v == 0) return 0;
	return as_val_tostring_callbacks[ v->type ](v);
}

