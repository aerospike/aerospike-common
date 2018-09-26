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

#pragma once

#include <aerospike/as_std.h>
#include <aerospike/as_val.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *	CONSTANTS
 *****************************************************************************/

/**
 *	INF value
 *	@ingroup aerospike_t
 */
AS_EXTERN extern const as_val as_cmp_inf;

/**
 *	WILDCARD value
 *	@ingroup aerospike_t
 */
AS_EXTERN extern const as_val as_cmp_wildcard;

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

AS_EXTERN void as_cmp_inf_val_destroy(as_val *v);
AS_EXTERN void as_cmp_wildcard_val_destroy(as_val *v);

AS_EXTERN uint32_t as_cmp_inf_val_hashcode(const as_val *v);
AS_EXTERN uint32_t as_cmp_wildcard_val_hashcode(const as_val *v);

AS_EXTERN char *as_cmp_inf_val_tostring(const as_val *v);
AS_EXTERN char *as_cmp_wildcard_val_tostring(const as_val *v);

#ifdef __cplusplus
} // end extern "C"
#endif
