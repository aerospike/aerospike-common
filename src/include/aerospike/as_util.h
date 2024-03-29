/* 
 * Copyright 2008-2022 Aerospike, Inc.
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

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * Calls a hook on a object.
 * If hook not found, then return the default value.
 */
#define as_util_hook(hook,default,object,...) \
	(object && object->hooks && object->hooks->hook ? object->hooks->hook(object, ##__VA_ARGS__) : default)

#define as_util_hook_ret_void(hook,object,...) \
	if (object && object->hooks && object->hooks->hook) object->hooks->hook(object, ##__VA_ARGS__)

/**
 * Converts from an as_val.
 */
#define as_util_fromval(object,type_id,type) \
	(object && as_val_type(object) == type_id ? (type *) object : NULL)

#ifdef __cplusplus
} // end extern "C"
#endif
