/* 
 * Copyright 2018 Aerospike, Inc.
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

#include <aerospike/as_msgpack_ext.h>

#include <citrusleaf/alloc.h>

/******************************************************************************
 *	CONSTANTS
 *****************************************************************************/

const as_val as_cmp_inf = {
	.type = AS_CMP_INF,
	.free = false,
	.count = 0
};

const as_val as_cmp_wildcard = {
	.type = AS_CMP_WILDCARD,
	.free = false,
	.count = 0
};

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

void
as_cmp_inf_val_destroy(as_val *v)
{
	return;
}

void
as_cmp_wildcard_val_destroy(as_val *v)
{
	return;
}

uint32_t
as_cmp_inf_val_hashcode(const as_val *v)
{
	return 0;
}

uint32_t
as_cmp_wildcard_val_hashcode(const as_val *v)
{
	return 0;
}

char *
as_cmp_inf_val_tostring(const as_val *v)
{
	return cf_strdup("INF");
}

char *
as_cmp_wildcard_val_tostring(const as_val *v)
{
	return cf_strdup("*");
}
