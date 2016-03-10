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
#include <stdbool.h>
#include <stddef.h>

/******************************************************************************
 * Types
 *****************************************************************************/

typedef struct as_random_s {
	uint64_t seed0;
	uint64_t seed1;
	bool initialized;
} as_random;

/******************************************************************************
 * Thread Local Variables
 *****************************************************************************/

static __thread as_random as_rand;

/******************************************************************************
 * Functions
 *****************************************************************************/

static inline void
as_random_init(as_random* random)
{
	// Initial seeds must be unique across all processes and threads.
	// Do not use a counter as a seed.
	random->seed0 = cf_get_rand64();
	random->seed1 = cf_get_rand64();
}

static inline as_random*
as_random_instance()
{
	as_random* r = &as_rand;

	if (! r->initialized) {
		as_random_init(r);
		r->initialized = true;
	}
	return r;
}

static inline uint64_t
as_random_next(as_random* random)
{
	// Use xorshift128+ algorithm.
	uint64_t s1 = random->seed0;
	const uint64_t s0 = random->seed1;
	random->seed0 = s0;
	s1 ^= s1 << 23;
	random->seed1 = (s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5));
	return random->seed1 + s0;
}

uint64_t
as_random_get_uint64()
{
	as_random* r = as_random_instance();
	return as_random_next(r);
}

uint32_t
as_random_get_uint32()
{
	as_random* r = as_random_instance();
	return (uint32_t)as_random_next(r);
}

void
as_random_get_bytes(uint8_t* bytes, uint32_t len)
{
	as_random* r = as_random_instance();
	uint8_t* p = bytes;
	uint8_t* end = bytes + len;
	
	while (p + sizeof(uint64_t) <= end) {
		// Append full 8 bytes
		*(uint64_t*)p = as_random_next(r);
		p += sizeof(uint64_t);
	}

	if (p < end) {
		// Append partial bytes.
		uint8_t tmp[sizeof(uint64_t)];
		uint8_t* t = tmp;
		*(uint64_t*)t = as_random_next(r);
		
		while (p < end) {
			*p++ = *t++;
		}
	}
}
