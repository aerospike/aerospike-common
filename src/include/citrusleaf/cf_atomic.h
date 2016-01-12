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
#pragma once

/**
 * SYNOPSIS
 * Atomic memory operations
 * Memory barriers
 *
 * cf_atomicX_add
 * Atomic addition: add a value b into an atomic integer a, and return the result
 **/

#include <stdint.h>
#include <citrusleaf/cf_arch.h>

#ifdef CF_WINDOWS
#include <intrin.h>
#include <WinSock2.h> // LONGLONG
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Concurrency kit needs to be under extern "C" when compiling C++.
#include <aerospike/ck/ck_pr.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

#ifdef MARCH_i686
#define SIZEOF_ATOMIC_INT 4
typedef volatile uint32_t cf_atomic32;
typedef volatile uint32_t cf_atomic_p;
typedef volatile uint32_t cf_atomic_int;
typedef uint32_t cf_atomic_int_t; // the point here is a type of the same size as cf_atomic_int but isn't atomic
#endif

#ifdef MARCH_x86_64
#define SIZEOF_ATOMIC_INT 8
typedef volatile uint64_t cf_atomic64;
typedef volatile uint32_t cf_atomic32;
typedef volatile uint64_t cf_atomic_p;
typedef volatile uint64_t cf_atomic_int;
typedef uint64_t cf_atomic_int_t; // the point here is a type of the same size as cf_atomic_int but isn't atomic
#endif

/******************************************************************************
 * MACROS
 *****************************************************************************/

#define cf_atomic32_get(a) (a)
#define cf_atomic32_set(a, b) (*(a) = (b))
#define cf_atomic32_sub(a,b) (cf_atomic32_add((a), (0 - (b))))
#define cf_atomic32_incr(a) (cf_atomic32_add((a), 1))
#define cf_atomic32_decr(a) (cf_atomic32_add((a), -1))

#ifdef MARCH_x86_64

#define cf_atomic64_get(a) (a)
#define cf_atomic64_set(a, b) (*(a) = (b))
#define cf_atomic64_sub(a,b) (cf_atomic64_add((a), (0 - (b))))
#define cf_atomic64_incr(a) (cf_atomic64_add((a), 1))
#define cf_atomic64_decr(a) (cf_atomic64_add((a), -1))

#define cf_atomic_p_get(_a) cf_atomic64_get(_a)
#define cf_atomic_p_set(_a, _b) cf_atomic64_set(_a, _b)
#define cf_atomic_p_add(_a, _b) cf_atomic64_add(_a, _b)
#define cf_atomic_p_incr(_a) cf_atomic64_add((_a), 1)
#define cf_atomic_p_decr(_a) cf_atomic64_add((_a), -1)

#define cf_atomic_int_get(_a) cf_atomic64_get(_a)
#define cf_atomic_int_set(_a, _b) cf_atomic64_set(_a, _b)
#define cf_atomic_int_add(_a, _b) cf_atomic64_add(_a, _b)
#define cf_atomic_int_sub(_a, _b) cf_atomic64_sub(_a, _b)
#define cf_atomic_int_incr(_a) cf_atomic64_add((_a), 1)
#define cf_atomic_int_decr(_a) cf_atomic64_add((_a), -1)

#else // ifndef MARCH_x86_64

#define cf_atomic_p_get(_a) cf_atomic32_get(_a)
#define cf_atomic_p_set(_a, _b) cf_atomic32_set(_a, _b)
#define cf_atomic_p_add(_a, _b) cf_atomic32_add(_a, _b)
#define cf_atomic_p_incr(_a) cf_atomic32_add((_a), 1)
#define cf_atomic_p_decr(_a) cf_atomic32_add((_a), -1)

#define cf_atomic_int_get(_a) cf_atomic32_get(_a)
#define cf_atomic_int_set(_a, _b) cf_atomic32_set(_a, _b)
#define cf_atomic_int_add(_a, _b) cf_atomic32_add(_a, _b)
#define cf_atomic_int_sub(_a, _b) cf_atomic32_sub(_a, _b)
#define cf_atomic_int_incr(_a) cf_atomic32_add((_a), 1)
#define cf_atomic_int_decr(_a) cf_atomic32_add((_a), -1)

#endif // ifdef MARCH_x86_64


#ifndef CF_WINDOWS
#ifdef MARCH_x86_64

#define cf_atomic_int_setmax(_a, _x) cf_atomic64_setmax(_a, _x)

#else // ifndef CF_WINDOWS && ifndef MARCH_x86_64

#define cf_atomic_p_setmax(_a, _x) cf_atomic32_setmax(_a, _x)

#define cf_atomic_int_setmax(_a, _x) cf_atomic32_setmax(_a, _x)

#endif // ifdef MARCH_x86_64
#endif // ifndef CF_WINDOWS


#ifdef CF_WINDOWS

#define smb_mb() _ReadWriteBarrier()

#else // ifndef CF_WINDOWS

#define smb_mb() ck_pr_fence_memory()
//#define smb_mb() asm volatile("mfence":::"memory")

#endif // CF_WINDOWS

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

static inline int64_t cf_atomic64_add(cf_atomic64 *, int64_t);
static inline int32_t cf_atomic32_add(cf_atomic32 *, int32_t);

/******************************************************************************
 * 64-BIT WINDOWS FUNCTIONS
 *****************************************************************************/

#ifdef CF_WINDOWS
#ifdef MARCH_x86_64

static inline int64_t cf_atomic64_add(cf_atomic64 *a, int64_t b) {
	int64_t i = b;
	b = _InterlockedExchangeAdd64((LONGLONG *)a, b);
	return(b + i);
}

#endif // ifdef MARCH_x86_64
#endif // ifdef CF_WINDOWS

/******************************************************************************
 * 64-BIT LINUX FUNCTIONS
 *****************************************************************************/

#ifndef CF_WINDOWS
#ifdef MARCH_x86_64

static inline int64_t cf_atomic64_add(cf_atomic64 *a, int64_t b) {
	return ck_pr_faa_64((uint64_t*)a, b) + b;
	//int64_t i = b;
	//__asm__ __volatile__ ("lock; xaddq %0, %1" : "+r" (b), "+m" (*a) : : "memory");
	//return(b + i);
}

static inline int64_t cf_atomic64_setmax(cf_atomic64 *a, int64_t x) {
	uint64_t prior;
	int64_t cur;

	// Get the current value of the atomic integer
	cur = cf_atomic64_get(*a);

	for ( ;; ) {
		// Check if the current value is equal to the criterion
		if (cur >= x)
			break;

		// Attempt compare-and-swap
		if (ck_pr_cas_64_value((uint64_t*)a, cur, x, &prior))
			break;

		// Operation failed, set cur to prior and go around again
		cur = prior;
	}

	return(cur != x);
}

#endif // ifdef  MARCH_x86_64
#endif // ifndef CF_WINDOWS

/******************************************************************************
 * 32-BIT WINDOWS FUNCTIONS
 *****************************************************************************/

#ifdef CF_WINDOWS

static inline int32_t cf_atomic32_add(cf_atomic32 *a, int32_t b){
	int32_t i = b;
	b = _InterlockedExchangeAdd((volatile long *)a, b);
	return(b + i);
}

#endif // ifdef CF_WINDOWS

/******************************************************************************
 * 32-BIT LINUX FUNCTIONS
 *****************************************************************************/

#ifndef CF_WINDOWS

static inline int32_t cf_atomic32_add(cf_atomic32 *a, int32_t b){
	return ck_pr_faa_32((uint32_t*)a, b) + b;
	//int32_t i = b;
	//__asm__ __volatile__ ("lock; xadd %0, %1" : "+r" (b), "+m" (*a) : : "memory");
	//return(b + i);
}

static inline int32_t cf_atomic32_setmax(cf_atomic32 *a, int32_t x) {
	uint32_t prior;
	int32_t cur;

	// Get the current value of the atomic integer
	cur = cf_atomic32_get(*a);

	for ( ;; ) {
		// Check if the current value is equal to the criterion
		if (cur >= x)
			break;

		// Attempt compare-and-swap
		if (ck_pr_cas_32_value((uint32_t*)a, cur, x, &prior))
			break;

		// Operation failed, set cur to prior and go around again
		cur = prior;
	}

	return(cur != x);
}

#endif // ifndef CF_WINDOWS

/*****************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
