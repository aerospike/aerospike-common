/* 
 * Copyright 2008-2020 Aerospike, Inc.
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

/******************************************************************************
 *
 * as_hashmap is DEPRECATED!
 *
 * Please switch to as_orderedmap where the base class as_map cannot be used.
 * as_hashmap will be removed in 2023.
 *
 ******************************************************************************/

#pragma once

#include <aerospike/as_map.h>
#include <aerospike/as_orderedmap.h>
#include <aerospike/as_std.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *	TYPES
 ******************************************************************************/

#define as_hashmap as_orderedmap

/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

static inline as_hashmap*
as_hashmap_init(as_hashmap* map, uint32_t capacity)
{
	map = as_orderedmap_init(map, capacity);

	if (map == NULL) {
		return NULL;
	}

	as_orderedmap_set_flags(map, 0);

	return map;
}

static inline as_hashmap*
as_hashmap_new(uint32_t capacity)
{
	as_orderedmap* map = as_orderedmap_new(capacity);

	if (map == NULL) {
		return NULL;
	}

	as_orderedmap_set_flags(map, 0);

	return map;
}

#define as_hashmap_destroy as_orderedmap_destroy

/*******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

#define as_hashmap_size as_orderedmap_size

static inline uint32_t
as_hashmap_hashcode(const as_hashmap* map)
{
	return 1;
}

/*******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 ******************************************************************************/

#define as_hashmap_get as_orderedmap_get
#define as_hashmap_set as_orderedmap_set
#define as_hashmap_clear as_orderedmap_clear
#define as_hashmap_remove as_orderedmap_remove
#define as_hashmap_set_flags as_orderedmap_set_flags

/******************************************************************************
 *	ITERATION FUNCTIONS
 *****************************************************************************/

#define as_hashmap_foreach as_orderedmap_foreach

#ifdef __cplusplus
} // end extern "C"
#endif
