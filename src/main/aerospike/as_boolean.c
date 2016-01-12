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
#include <aerospike/as_boolean.h>
#include <citrusleaf/alloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	as_boolean * boolean = (as_boolean *) cf_malloc(sizeof(as_boolean));
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
	char * str = (char *) cf_malloc(sizeof(char) * 6);
    if (!str) return str;
	bzero(str,6);
	if ( b->value ) {
		strcpy(str,"true");
	}
	else {
		strcpy(str,"false");
	}
	return str;

}
