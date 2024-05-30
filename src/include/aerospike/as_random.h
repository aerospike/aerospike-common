/*
 * Copyright 2008-2024 Aerospike, Inc.
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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Types
 *****************************************************************************/

/**
 * Random seeds used in xorshift128+ algorithm: http://xorshift.di.unimi.it
 * Not thread-safe.  Instantiate once per thread.
 */
typedef struct as_random_s {
	uint64_t seed0;
	uint64_t seed1;
	bool initialized;
} as_random;

/*******************************************************************************
 * Functions
 ******************************************************************************/

/**
 * Initialize random instance.
 */
AS_EXTERN void
as_random_init(as_random* random);

/**
 * Get thread local random instance.
 */
AS_EXTERN as_random*
as_random_instance(void);

/**
 * Get random unsigned 64 bit integer from given as_random instance
 * using xorshift128+ algorithm: http://xorshift.di.unimi.it
 */
static inline uint64_t
as_random_next_uint64(as_random* random)
{
	// Use xorshift128+ algorithm.
	uint64_t s1 = random->seed0;
	const uint64_t s0 = random->seed1;
	random->seed0 = s0;
	s1 ^= s1 << 23;
	random->seed1 = (s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5));
	return random->seed1 + s0;
}

/**
 * Get random unsigned 32 bit integer from given as_random instance.
 */
static inline uint32_t
as_random_next_uint32(as_random* random)
{
	return (uint32_t)as_random_next_uint64(random);
}

/**
 * Get random unsigned 64 bit integer from thread local instance.
 */
static inline uint64_t
as_random_get_uint64(void)
{
	as_random* random = as_random_instance();
	return as_random_next_uint64(random);
}

/**
 * Get random unsigned 32 bit integer from thread local instance.
 */
static inline uint32_t
as_random_get_uint32(void)
{
	return (uint32_t)as_random_get_uint64();
}

/**
 * Get random bytes of specified length from given as_random instance.
 */
AS_EXTERN void
as_random_next_bytes(as_random* random, uint8_t* bytes, uint32_t len);

/**
 * Get random bytes of specified length from thread local instance.
 */
static inline void
as_random_get_bytes(uint8_t* bytes, uint32_t len)
{
	as_random* random = as_random_instance();
	as_random_next_bytes(random, bytes, len);
}

/**
 * Get null terminated random alphanumeric string of specified length using given
 * as_random instance. String buffer must include space for extra null byte.
 */
AS_EXTERN void
as_random_next_str(as_random* random, char* str, uint32_t len);

/**
 * Get null terminated random alphanumeric string of specified length from thread
 * local random instance. String buffer must include space for extra null byte.
 */
static inline void
as_random_get_str(char* str, uint32_t len)
{
	as_random* random = as_random_instance();
	as_random_next_str(random, str, len);
}

#ifdef __cplusplus
} // end extern "C"
#endif
