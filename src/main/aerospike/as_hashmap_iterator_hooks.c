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

#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
 *	EXTERN FUNCTIONS
 *****************************************************************************/

extern bool as_hashmap_iterator_release(as_hashmap_iterator * iterator);

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

static bool _as_hashmap_iterator_destroy(as_iterator * i) 
{
	return as_hashmap_iterator_release((as_hashmap_iterator *) i);
}

static bool _as_hashmap_iterator_has_next(const as_iterator * i) 
{
	return as_hashmap_iterator_has_next((const as_hashmap_iterator *) i);
}

static const as_val * _as_hashmap_iterator_next(as_iterator * i) 
{
	return as_hashmap_iterator_next((as_hashmap_iterator *) i);
}

/******************************************************************************
 *	HOOKS
 *****************************************************************************/

const as_iterator_hooks as_hashmap_iterator_hooks = {
	.destroy    = _as_hashmap_iterator_destroy,
	.has_next   = _as_hashmap_iterator_has_next,
	.next       = _as_hashmap_iterator_next
};
