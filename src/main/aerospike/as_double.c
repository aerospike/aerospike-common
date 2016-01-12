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
#include <aerospike/as_double.h>
#include <citrusleaf/alloc.h>
#include <stdio.h>

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

static as_double*
as_double_cons(as_double* value_ptr, bool free, double value)
{
	if (!value_ptr ) {
		return value_ptr;
	}

	as_val_cons((as_val*)value_ptr, AS_DOUBLE, free);
	value_ptr->value = value;
	return value_ptr;
}

as_double*
as_double_init(as_double* value_ptr, double value)
{
	return as_double_cons(value_ptr, false, value);
}

as_double*
as_double_new(double value)
{
	as_double* value_ptr = (as_double*) cf_malloc(sizeof(as_double));
	return as_double_cons(value_ptr, true, value);
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

void
as_double_val_destroy(as_val* val)
{
	as_double* value_ptr = as_double_fromval(val);
	
	if (!value_ptr ) {
		return;
	}
	value_ptr->value = 0.0;
}

uint32_t
as_double_val_hashcode(const as_val* val)
{
	as_double* value_ptr = as_double_fromval(val);
	
	if (!value_ptr) {
		return 0;
	}
	
	uint64_t v = *(uint64_t*)&value_ptr->value;
	return (uint32_t)(v ^ (v >> 32));
}

char*
as_double_val_tostring(const as_val * val)
{
	as_double* value_ptr = (as_double*)val;
	char* str = (char*)cf_malloc(sizeof(char) * 64);
	sprintf(str, "%.16g", value_ptr->value);
	return str;
}
