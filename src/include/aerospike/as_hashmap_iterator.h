/* 
 * Copyright 2008-2018 Aerospike, Inc.
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

#include <aerospike/as_hashmap.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_std.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *	TYPES
 ******************************************************************************/

#define as_hashmap_iterator as_orderedmap_iterator

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

#define as_hashmap_iterator_init as_orderedmap_iterator_init
#define as_hashmap_iterator_new as_orderedmap_iterator_new
#define as_hashmap_iterator_destroy as_orderedmap_iterator_destroy

/******************************************************************************
 *	ITERATOR FUNCTIONS
 *****************************************************************************/

#define as_hashmap_iterator_has_next as_orderedmap_iterator_has_next
#define as_hashmap_iterator_next as_orderedmap_iterator_next

#ifdef __cplusplus
} // end extern "C"
#endif
