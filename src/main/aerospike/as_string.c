/* 
 * Copyright 2008-2014 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

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

#ifdef CF_WINDOWS
#define FILE_SEPARATOR '\\'
#else
#define FILE_SEPARATOR '/'
#endif

const char* as_basename(as_string * filename, const char* path)
{
	if (path == 0 || *path == 0) {
		// Found empty string.  Return current directory constant.
		char* value = ".";
		as_string_cons(filename, false, value, false);
		return value;
	}
	
	const char* p = path;
	const char* begin = 0;

	// Skip till end of string.
	while (*p) {
		if (*p == FILE_SEPARATOR) {
			begin = p + 1;
		}
		p++;
	}
	
	if (begin == 0) {
		// No slashes found.  Return original string.
		as_string_cons(filename, false, (char*)path, false);
		return path;
	}
	
	if (begin == p) {
		// Found trailing slashes.
		// Create new string to hold filename without slashes.
		p--;
		
		while (*p == FILE_SEPARATOR) {
			if (p == path) {
				// String contains all slashes. Return slash constant.
				char* value = "/";
				as_string_cons(filename, false, value, false);
				return value;
			}
			p--;
		}
		const char* end = p;
				
		while (p != path && *p != FILE_SEPARATOR) {
			p--;
		}
		
		if (*p == FILE_SEPARATOR) {
			p++;
		}
		
		int len = (int)(end - p) + 1;
		char* str = cf_malloc(len + 1);
		memcpy(str, p, len);
		*(str + len) = 0;
		as_string_cons(filename, false, str, true);
		return str;
	}
	
	// Return begin of filename.
	as_string_cons(filename, false, (char*)begin, false);
	return begin;
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

/******************************************************************************
 *	String utilities
 ******************************************************************************/

bool
as_strncpy(char* trg, const char* src, int size)
{
	int max = size - 1;
	int i = 0;
	
	while (*src) {
		if (i >= max) {
			*trg = 0;
			return true;
		}
		*trg++ = *src++;
		i++;
	}
	*trg = 0;
	return false;
}
