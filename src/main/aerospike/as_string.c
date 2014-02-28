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
#include <string.h>

#include <citrusleaf/alloc.h>

#include <aerospike/as_string.h>

/******************************************************************************
 *	INLINE FUNCTIONS
 *****************************************************************************/

extern inline void			as_string_destroy(as_string * string);

extern inline char *		as_string_getorelse(const as_string * string, char * fallback);
extern inline char *		as_string_get(const as_string * string);
extern inline char *		as_string_tostring(const as_string * string);

extern inline as_val *		as_string_toval(const as_string * string);
extern inline as_string *	as_string_fromval(const as_val * v);

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

static inline as_string * as_string_cons(as_string * string, bool free, char * value, bool value_free)
{
	if ( !string ) return string;

	as_val_cons((as_val *) string, AS_STRING, free);
	string->free = value_free;
	string->value = value;
	string->len = SIZE_MAX;
	return string;
}

as_string * as_string_init(as_string * string, char * value, bool free)
{
	return as_string_cons(string, false, value, free);
}

as_string * as_string_new(char * value, bool free)
{
	as_string * string = (as_string *) cf_malloc(sizeof(as_string));
	return as_string_cons(string, true, value, free);
}

as_string *as_string_new_strdup(const char * s)
{
	return as_string_new(cf_strdup(s), true);
}

/******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

size_t as_string_len(as_string * string)
{
	if (string->value == NULL) {
		return 0;
	}
	if (string->len == SIZE_MAX) {
		string->len = strlen(string->value);
	}
	return string->len;
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

void as_string_val_destroy(as_val * v)
{
	as_string * string = as_string_fromval(v);
	if ( !string ) return;

	if ( string->value && string->free ) {
		cf_free(string->value);
	}
	
	string->value = NULL;
	string->free = false;
}

uint32_t as_string_val_hashcode(const as_val * v)
{
	as_string * string = as_string_fromval(v);
	if ( string == NULL || string->value == NULL) return 0;
	uint32_t hash = 0;
	int c;
	char * str = string->value;
	while ( (c = *str++) ) {
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

char * as_string_val_tostring(const as_val * v)
{
	as_string * s = (as_string *) v;
	if (s->value == NULL) return(NULL);
	size_t sl = as_string_len(s);
	size_t st = 3 + sl;
	char * str = (char *) cf_malloc(sizeof(char) * st);
	if (!str) return str;
	*(str + 0) = '\"';
	strcpy(str + 1, s->value);
	*(str + 1 + sl) = '\"';
	*(str + 1 + sl + 1) = '\0';
	return str;
}
