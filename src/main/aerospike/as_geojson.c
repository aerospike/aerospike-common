/* 
 * Copyright 2008-2016 Aerospike, Inc.
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

#include <aerospike/as_geojson.h>

/******************************************************************************
 *	INLINE FUNCTIONS
 *****************************************************************************/

extern inline void			as_geojson_destroy(as_geojson * string);

extern inline char *		as_geojson_getorelse(const as_geojson * string, char * fallback);
extern inline char *		as_geojson_get(const as_geojson * string);

extern inline as_val *		as_geojson_toval(const as_geojson * string);
extern inline as_geojson *	as_geojson_fromval(const as_val * v);

/******************************************************************************
 *	INSTANCE FUNCTIONS
 *****************************************************************************/

static inline as_geojson * as_geojson_cons(as_geojson * string, bool free, char * value, size_t len, bool value_free)
{
	if ( !string ) return string;

	as_val_cons((as_val *) string, AS_GEOJSON, free);
	string->free = value_free;
	string->value = value;
	string->len = len;
	return string;
}

as_geojson * as_geojson_init(as_geojson * string, char * value, bool free)
{
	return as_geojson_cons(string, false, value, SIZE_MAX, free);
}

as_geojson * as_geojson_init_wlen(as_geojson * string, char * value, size_t len, bool free)
{
	return as_geojson_cons(string, false, value, len, free);
}

as_geojson * as_geojson_new(char * value, bool free)
{
	as_geojson * string = (as_geojson *) cf_malloc(sizeof(as_geojson));
	return as_geojson_cons(string, true, value, SIZE_MAX, free);
}

as_geojson * as_geojson_new_wlen(char * value, size_t len, bool free)
{
	as_geojson * string = (as_geojson *) cf_malloc(sizeof(as_geojson));
	return as_geojson_cons(string, true, value, len, free);
}

as_geojson *as_geojson_new_strdup(const char * s)
{
	return as_geojson_new(cf_strdup(s), true);
}

/******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

size_t as_geojson_len(as_geojson * string)
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

void as_geojson_val_destroy(as_val * v)
{
	as_geojson * string = as_geojson_fromval(v);
	if ( !string ) return;

	if ( string->value && string->free ) {
		cf_free(string->value);
	}
	
	string->value = NULL;
	string->free = false;
}

uint32_t as_geojson_val_hashcode(const as_val * v)
{
	as_geojson * string = as_geojson_fromval(v);
	if ( string == NULL || string->value == NULL) return 0;
	uint32_t hash = 0;
	int c;
	char * str = string->value;
	while ( (c = *str++) ) {
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

char * as_geojson_val_tostring(const as_val * v)
{
	as_geojson * s = (as_geojson *) v;
	if (s->value == NULL) return(NULL);
	size_t sl = as_geojson_len(s);
	size_t st = 3 + sl;
	char * str = (char *) cf_malloc(sizeof(char) * st);
	if (!str) return str;
	*(str + 0) = '\"';
	strcpy(str + 1, s->value);
	*(str + 1 + sl) = '\"';
	*(str + 1 + sl + 1) = '\0';
	return str;
}
