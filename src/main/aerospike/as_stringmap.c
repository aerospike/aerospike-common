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

#include <aerospike/as_stringmap.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <aerospike/as_val.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>

/******************************************************************************
 * SETTER FUNCTIONS
 *****************************************************************************/

/**
 * Set the specified key's value to an as_val.
 */
extern inline int as_stringmap_set(as_map * m, const char * k, as_val * v);

/**
 * Set the specified key's value to an int64_t.
 */
extern inline int as_stringmap_set_int64(as_map * m, const char * k, int64_t v);

/**
 * Set the specified key's value to a NULL terminated string.
 */
extern inline int as_stringmap_set_str(as_map * m, const char * k, const char * v);

/**
 * Set the specified key's value to an as_integer.
 */
extern inline int as_stringmap_set_integer(as_map * m, const char * k, as_integer * v);
/**
 * Set the specified key's value to an as_string.
 */
extern inline int as_stringmap_set_string(as_map * m, const char * k, as_string * v);

/**
 * Set the specified key's value to an as_bytes.
 */
extern inline int as_stringmap_set_bytes(as_map * m, const char * k, as_bytes * v);

/**
 * Set the specified key's value to an as_list.
 */
extern inline int as_stringmap_set_list(as_map * m, const char * k, as_list * v);

/**
 * Set the specified key's value to an as_map.
 */
extern inline int as_stringmap_set_map(as_map * m, const char * k, as_map * v);

/******************************************************************************
 * GETTER FUNCTIONS
 *****************************************************************************/

/**
 * Get the specified key's value as an as_val.
 */
extern inline as_val * as_stringmap_get(as_map * m, const char * k);

/**
 * Get the specified key's value as an int64_t.
 */
extern inline int64_t as_stringmap_get_int64(as_map * m, const char * k);

/**
 * Get the specified key's value as a NULL terminated string.
 */
extern inline char * as_stringmap_get_str(as_map * m, const char * k);

/**
 * Get the specified key's value as an as_integer.
 */
extern inline as_integer * as_stringmap_get_integer(as_map * m, const char * k);

/**
 * Get the specified key's value as an as_string.
 */
extern inline as_string * as_stringmap_get_string(as_map * m, const char * k);

/**
 * Get the specified key's value as an as_bytes.
 */
extern inline as_bytes * as_stringmap_get_bytes(as_map * m, const char * k);

/**
 * Get the specified key's value as an as_list.
 */
extern inline as_list * as_stringmap_get_list(as_map * m, const char * k);

/**
 * Get the specified key's value as an as_map.
 */
extern inline as_map * as_stringmap_get_map(as_map * m, const char * k);

