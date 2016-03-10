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
#include <aerospike/as_random.h>
#include <citrusleaf/cf_random.h>
#include <stddef.h>

/******************************************************************************
 * Thread Local Variables
 *****************************************************************************/

__thread as_random as_rand;

/******************************************************************************
 * Functions
 *****************************************************************************/

void
as_random_init(as_random* random)
{
	// Keep this method in C file, so cf_random.h can stay a private header.
	// Initial seeds must be unique across all processes and threads.
	// Do not use a counter as a seed.
	random->seed0 = cf_get_rand64();
	random->seed1 = cf_get_rand64();
	random->initialized = true;
}

void
as_random_next_bytes(as_random* random, uint8_t* bytes, uint32_t len)
{
	uint8_t* p = bytes;
	uint8_t* end = bytes + len;
	
	while (p + sizeof(uint64_t) <= end) {
		// Append full 8 bytes
		*(uint64_t*)p = as_random_next_uint64(random);
		p += sizeof(uint64_t);
	}

	if (p < end) {
		// Append partial bytes.
		uint8_t tmp[sizeof(uint64_t)];
		uint8_t* t = tmp;
		*(uint64_t*)t = as_random_next_uint64(random);
		
		while (p < end) {
			*p++ = *t++;
		}
	}
}

uint64_t
as_random_get_uint64()
{
	as_random* random = as_random_instance();
	return as_random_next_uint64(random);
}
