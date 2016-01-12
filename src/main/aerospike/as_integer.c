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
#include <aerospike/as_integer.h>
#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	as_integer * integer = (as_integer *) cf_malloc(sizeof(as_integer));
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
	return i != NULL ? (uint32_t)i->value : 0;
}

char * as_integer_val_tostring(const as_val * v)
{
	as_integer * i = (as_integer *) v;
	char * str = (char *) cf_malloc(sizeof(char) * 32);
	bzero(str, 32);
	sprintf(str, "%" PRId64, i->value);
	return str;
}
