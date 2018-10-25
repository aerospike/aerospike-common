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
#pragma once
#ifndef CF_ATOMIC_H
#define CF_ATOMIC_H

/**
 * SYNOPSIS
 * Atomic memory operations
 * Memory barriers
 *
 * cf_atomicX_add
 * Atomic addition: add a value b into an atomic integer a, and return the result
 **/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GCC_ATOMICS

typedef uint64_t cf_atomic64;
typedef uint32_t cf_atomic32;
typedef uint64_t cf_atomic_p;
typedef uint64_t cf_atomic_int;

#define cf_atomic32_get(a) __atomic_load_n((a), __ATOMIC_SEQ_CST)
#define cf_atomic32_set(a, b) __atomic_store_n((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic32_sub(a, b) __atomic_sub_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic32_add(a, b) __atomic_add_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic32_incr(a)  __atomic_add_fetch((a), 1, __ATOMIC_SEQ_CST)
#define cf_atomic32_decr(a)  __atomic_sub_fetch((a), 1, __ATOMIC_SEQ_CST)

#define cf_atomic64_get(a) __atomic_load_n((a), __ATOMIC_SEQ_CST)
#define cf_atomic64_set(a, b) __atomic_store_n((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic64_sub(a, b) __atomic_sub_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic64_add(a, b) __atomic_add_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic64_incr(a)  __atomic_add_fetch((a), 1, __ATOMIC_SEQ_CST)
#define cf_atomic64_decr(a)  __atomic_sub_fetch((a), 1, __ATOMIC_SEQ_CST)
#define cf_atomic64_cas(a, b, c) __atomic_compare_exchange_n((a), (b), (c), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define cf_atomic_p_get(a) __atomic_load_n((a), __ATOMIC_SEQ_CST)
#define cf_atomic_p_set(a, b) __atomic_store_n((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic_p_sub(a, b) __atomic_sub_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic_p_add(a, b) __atomic_add_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic_p_incr(a)  __atomic_add_fetch((a), 1, __ATOMIC_SEQ_CST)
#define cf_atomic_p_decr(a)  __atomic_sub_fetch((a), 1, __ATOMIC_SEQ_CST)
#define cf_atomic_p_cas(a, b, c) __atomic_compare_exchange_n((a), (b), (c), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define cf_atomic_int_get(a) __atomic_load_n((a), __ATOMIC_SEQ_CST)
#define cf_atomic_int_set(a, b) __atomic_store_n((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic_int_sub(a, b) __atomic_sub_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic_int_add(a, b) __atomic_add_fetch((a), (b), __ATOMIC_SEQ_CST)
#define cf_atomic_int_incr(a)  __atomic_add_fetch((a), 1, __ATOMIC_SEQ_CST)
#define cf_atomic_int_decr(a)  __atomic_sub_fetch((a), 1, __ATOMIC_SEQ_CST)

static inline int cf_atomic32_setmax(cf_atomic32 *a, int32_t x) {
	// Get the current value of the atomic integer.
	int32_t cur = cf_atomic32_get(a);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		if (__atomic_compare_exchange_n(a, &cur, x, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
			return 1;
		}
		// Current value had changed, go around again.
	}

	// Proposed value not swapped in as new maximum.
	return 0;
}

static inline int cf_atomic64_setmax(cf_atomic64 *a, int64_t x) {
	// Get the current value of the atomic integer.

	int32_t cur = cf_atomic64_get(a);
	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		if (__atomic_compare_exchange_n(a, &cur, x, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
			return 1;
		}
		// Current value had changed, go around again.
	}

	// Proposed value not swapped in as new maximum.
	return 0;
}

#else


// Concurrency kit needs to be under extern "C" when compiling C++.
#include <aerospike/ck/ck_pr.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

#define SIZEOF_ATOMIC_INT 8
typedef volatile uint64_t cf_atomic64;
typedef volatile uint32_t cf_atomic32;
typedef volatile uint64_t cf_atomic_p;
typedef volatile uint64_t cf_atomic_int;
typedef uint64_t cf_atomic_int_t; // the point here is a type of the same size as cf_atomic_int but isn't atomic

/******************************************************************************
 * MACROS
 *****************************************************************************/

static inline cf_atomic32 cf_xx_atomic32_get(const cf_atomic32 *a) { return *a; }
#define cf_atomic32_get(a) cf_xx_atomic32_get((const cf_atomic32 *) a)
#define cf_atomic32_set(a, b) (*(a) = (b))
#define cf_atomic32_sub(a, b) ((cf_atomic32)cf_atomic32_add((a), (0 - (b))))
#define cf_atomic32_incr(a) ((cf_atomic32)cf_atomic32_add((a), 1))
#define cf_atomic32_decr(a) ((cf_atomic32)cf_atomic32_add((a), -1))

static inline cf_atomic64 cf_xx_atomic64_get(const cf_atomic64 *a) { return *a; }
#define cf_atomic64_get(a) cf_xx_atomic64_get((const cf_atomic64 *) a)
#define cf_atomic64_set(a, b) (*(a) = (b))
#define cf_atomic64_sub(a, b) ((cf_atomic64)cf_atomic64_add((a), (0 - (b))))
#define cf_atomic64_incr(a) ((cf_atomic64)cf_atomic64_add((a), 1))
#define cf_atomic64_decr(a) ((cf_atomic64)cf_atomic64_add((a), -1))
#define cf_atomic64_cas(a, b, c) (ck_pr_cas_64((a), (*b), (c)))

#define cf_atomic_p_get(a) (*a)
#define cf_atomic_p_set(_a, _b) cf_atomic64_set(_a, _b)
#define cf_atomic_p_add(_a, _b) cf_atomic64_add(_a, _b)
#define cf_atomic_p_incr(_a) cf_atomic64_add((_a), 1)
#define cf_atomic_p_decr(_a) cf_atomic64_add((_a), -1)
#define cf_atomic_p_cas(a, b, c) __atomic_compare_exchange_n((a), (b), (c), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

static inline cf_atomic_int cf_xx_atomic_int_get(const cf_atomic_int *a) { return *a; }
#define cf_atomic_int_get(a) cf_xx_atomic_int_get((const cf_atomic_int *) a)
#define cf_atomic_int_set(_a, _b) cf_atomic64_set(_a, _b)
#define cf_atomic_int_add(_a, _b) cf_atomic64_add(_a, _b)
#define cf_atomic_int_sub(_a, _b) cf_atomic64_sub(_a, _b)
#define cf_atomic_int_incr(_a) cf_atomic64_add((_a), 1)
#define cf_atomic_int_decr(_a) cf_atomic64_add((_a), -1)

/******************************************************************************
 * LINUX FUNCTIONS
 *****************************************************************************/

#define cf_atomic_int_setmax(_a, _x) cf_atomic64_setmax(_a, _x)

#define smb_mb() ck_pr_fence_memory()

static inline int64_t cf_atomic64_add(cf_atomic64 *a, int64_t b) {
	return ck_pr_faa_64((uint64_t*)a, b) + b;
	//int64_t i = b;
	//__asm__ __volatile__ ("lock; xaddq %0, %1" : "+r" (b), "+m" (*a) : : "memory");
	//return(b + i);
}

static inline bool cf_atomic64_setmax(cf_atomic64 *a, int64_t x) {
	uint64_t prior;

	// Get the current value of the atomic integer.
	int64_t cur = cf_atomic64_get(a);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		if (ck_pr_cas_64_value((uint64_t*)a, cur, x, &prior)) {
			// Current value was unchanged, proposed value swapped in.
			return true;
		}

		// Current value had changed, set cur to prior and go around again.
		cur = prior;
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

static inline int32_t cf_atomic32_add(cf_atomic32 *a, int32_t b){
	return ck_pr_faa_32((uint32_t*)a, b) + b;
	//int32_t i = b;
	//__asm__ __volatile__ ("lock; xadd %0, %1" : "+r" (b), "+m" (*a) : : "memory");
	//return(b + i);
}

static inline bool cf_atomic32_setmax(cf_atomic32 *a, int32_t x) {
	uint32_t prior;

	// Get the current value of the atomic integer.
	int32_t cur = cf_atomic32_get(a);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		if (ck_pr_cas_32_value((uint32_t*)a, cur, x, &prior)) {
			return true;
		}

		// Current value had changed, set cur to prior and go around again.
		cur = prior;
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

#endif // GCC_ATOMICS

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // CF_ATOMIC_H
