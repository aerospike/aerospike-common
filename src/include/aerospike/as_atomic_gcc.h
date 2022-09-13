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

#include <aerospike/as_std.h>

#ifdef __cplusplus
extern "C" {
#endif

// Concurrency kit needs to be under extern "C" when compiling C++.
#include <aerospike/ck/ck_spinlock.h>
#include <aerospike/ck/ck_swlock.h>

/******************************************************************************
 * LOAD
 *****************************************************************************/

// type as_load_acq(const type* target)
#define as_load_acq(_target) __atomic_load_n(_target, __ATOMIC_ACQUIRE)

// type as_load_rlx(const type* target)
#define as_load_rlx(_target) __atomic_load_n(_target, __ATOMIC_RELAXED)

// type as_load_seq(const type* target)
#define as_load_seq(_target) __atomic_load_n(_target, __ATOMIC_SEQ_CST)

static inline void*
as_load_ptr(void* const* target)
{
	return as_load_rlx(target);
}

static inline uint64_t
as_load_uint64(const uint64_t* target)
{
	return as_load_rlx(target);
}

static inline int64_t
as_load_int64(const int64_t* target)
{
	return as_load_rlx(target);
}

static inline uint32_t
as_load_uint32(const uint32_t* target)
{
	return as_load_rlx(target);
}

static inline int32_t
as_load_int32(const int32_t* target)
{
	return as_load_rlx(target);
}

static inline uint16_t
as_load_uint16(const uint16_t* target)
{
	return as_load_rlx(target);
}

static inline int16_t
as_load_int16(const int16_t* target)
{
	return as_load_rlx(target);
}

static inline uint8_t
as_load_uint8(const uint8_t* target)
{
	return as_load_rlx(target);
}

static inline int8_t
as_load_int8(const int8_t* target)
{
	return as_load_rlx(target);
}

/******************************************************************************
 * STORE
 *****************************************************************************/

// void as_store_rls(type* target, type value)
#define as_store_rls(_target, _value) __atomic_store_n(_target, _value, __ATOMIC_RELEASE)

// void as_store_rlx(type* target, type value)
#define as_store_rlx(_target, _value) __atomic_store_n(_target, _value, __ATOMIC_RELAXED)

// void as_store_seq(type* target, type value)
#define as_store_seq(_target, _value) __atomic_store_n(_target, _value, __ATOMIC_SEQ_CST)

static inline void
as_store_ptr(void** target, void* value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_uint64(uint64_t* target, uint64_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_int64(int64_t* target, int64_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_uint32(uint32_t* target, uint32_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_int32(int32_t* target, int32_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_uint16(uint16_t* target, uint16_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_int16(int16_t* target, int16_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_uint8(uint8_t* target, uint8_t value)
{
	as_store_rlx(target, value);
}

static inline void
as_store_int8(int8_t* target, int8_t value)
{
	as_store_rlx(target, value);
}

/******************************************************************************
 * FETCH AND ADD
 *****************************************************************************/

// type as_faa_acq(type* target, type value)
#define as_faa_acq(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_ACQUIRE)

// type as_faa_rls(type* target, type value)
#define as_faa_rls(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_RELEASE)

// type as_faa_rlx(type* target, type value)
#define as_faa_rlx(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_RELAXED)

// type as_faa_seq(type* target, type value)
#define as_faa_seq(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_SEQ_CST)

static inline uint64_t
as_faa_uint64(uint64_t* target, int64_t value)
{
	return as_faa_rlx(target, value);
}

static inline int64_t
as_faa_int64(int64_t* target, int64_t value)
{
	return as_faa_rlx(target, value);
}

static inline uint32_t
as_faa_uint32(uint32_t* target, int32_t value)
{
	return as_faa_rlx(target, value);
}

static inline int32_t
as_faa_int32(int32_t* target, int32_t value)
{
	return as_faa_rlx(target, value);
}

static inline uint16_t
as_faa_uint16(uint16_t* target, int16_t value)
{
	return as_faa_rlx(target, value);
}

static inline int16_t
as_faa_int16(int16_t* target, int16_t value)
{
	return as_faa_rlx(target, value);
}

/******************************************************************************
 * ADD AND FETCH
 *****************************************************************************/

// type as_aaf_acq(type* target, type value)
#define as_aaf_acq(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_ACQUIRE)

// type as_aaf_rls(type* target, type value)
#define as_aaf_rls(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_RELEASE)

// type as_aaf_rlx(type* target, type value)
#define as_aaf_rlx(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_RELAXED)

// type as_aaf_seq(type* target, type value)
#define as_aaf_seq(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_SEQ_CST)

static inline uint64_t
as_aaf_uint64(uint64_t* target, int64_t value)
{
	return as_aaf_rlx(target, value);
}

static inline int64_t
as_aaf_int64(int64_t* target, int64_t value)
{
	return as_aaf_rlx(target, value);
}

static inline uint32_t
as_aaf_uint32(uint32_t* target, int32_t value)
{
	return as_aaf_rlx(target, value);
}

static inline int32_t
as_aaf_int32(int32_t* target, int32_t value)
{
	return as_aaf_rlx(target, value);
}

static inline uint16_t
as_aaf_uint16(uint16_t* target, int16_t value)
{
	return as_aaf_rlx(target, value);
}

static inline int16_t
as_aaf_int16(int16_t* target, int16_t value)
{
	return as_aaf_rlx(target, value);
}

/******************************************************************************
 * ADD
 *****************************************************************************/

static inline void
as_add_uint64(uint64_t* target, int64_t value)
{
	as_faa_rlx(target, value);
}

static inline void
as_add_int64(int64_t* target, int64_t value)
{
	as_faa_rlx(target, value);
}

static inline void
as_add_uint32(uint32_t* target, int32_t value)
{
	as_faa_rlx(target, value);
}

static inline void
as_add_int32(int32_t* target, int32_t value)
{
	as_faa_rlx(target, value);
}

static inline void
as_add_uint16(uint16_t* target, int16_t value)
{
	as_faa_rlx(target, value);
}

static inline void
as_add_int16(int16_t* target, int16_t value)
{
	as_faa_rlx(target, value);
}

/******************************************************************************
 * INCREMENT
 *****************************************************************************/

static inline void
as_incr_uint64(uint64_t* target)
{
	as_faa_rlx(target, 1);
}

static inline void
as_incr_int64(int64_t* target)
{
	as_faa_rlx(target, 1);
}

static inline void
as_incr_uint32(uint32_t* target)
{
	as_faa_rlx(target, 1);
}

static inline void
as_incr_int32(int32_t* target)
{
	as_faa_rlx(target, 1);
}

static inline void
as_incr_uint16(uint16_t* target)
{
	as_faa_rlx(target, 1);
}

static inline void
as_incr_int16(int16_t* target)
{
	as_faa_rlx(target, 1);
}

/******************************************************************************
 * DECREMENT
 *****************************************************************************/

static inline void
as_decr_uint64(uint64_t* target)
{
	as_faa_rlx(target, -1);
}

static inline void
as_decr_int64(int64_t* target)
{
	as_faa_rlx(target, -1);
}

static inline void
as_decr_uint32(uint32_t* target)
{
	as_faa_rlx(target, -1);
}

static inline void
as_decr_int32(int32_t* target)
{
	as_faa_rlx(target, -1);
}

static inline void
as_decr_uint16(uint16_t* target)
{
	as_faa_rlx(target, -1);
}

static inline void
as_decr_int16(int16_t* target)
{
	as_faa_rlx(target, -1);
}

/******************************************************************************
 * FETCH AND SWAP
 *****************************************************************************/

// type as_fas_acq(type* target, type value)
#define as_fas_acq(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_ACQUIRE)

// type as_fas_rls(type* target, type value)
#define as_fas_rls(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_RELEASE)

// type as_fas_rlx(type* target, type value)
#define as_fas_rlx(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_RELAXED)

// type as_fas_seq(type* target, type value)
#define as_fas_seq(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_SEQ_CST)

static inline uint64_t
as_fas_uint64(uint64_t* target, uint64_t value)
{
	return as_fas_rlx(target, value);
}

static inline int64_t
as_fas_int64(int64_t* target, int64_t value)
{
	return as_fas_rlx(target, value);
}

static inline uint32_t
as_fas_uint32(uint32_t* target, uint32_t value)
{
	return as_fas_rlx(target, value);
}

static inline int32_t
as_fas_int32(int32_t* target, int32_t value)
{
	return as_fas_rlx(target, value);
}

static inline uint16_t
as_fas_uint16(uint16_t* target, uint16_t value)
{
	return as_fas_rlx(target, value);
}

static inline int16_t
as_fas_int16(int16_t* target, int16_t value)
{
	return as_fas_rlx(target, value);
}

/******************************************************************************
 * COMPARE AND SWAP
 *****************************************************************************/

// bool as_cas_acq(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_acq(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

// bool as_cas_rls(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_rls(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)

// bool as_cas_rlx(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_rlx(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

// bool as_cas_seq(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_seq(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)

static inline bool
as_cas_uint64(uint64_t* target, uint64_t old_value, uint64_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

static inline bool
as_cas_int64(int64_t* target, int64_t old_value, int64_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

static inline bool
as_cas_uint32(uint32_t* target, uint32_t old_value, uint32_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

static inline bool
as_cas_int32(int32_t* target, int32_t old_value, int32_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

static inline bool
as_cas_uint16(uint16_t* target, uint16_t old_value, uint16_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

static inline bool
as_cas_int16(int16_t* target, int16_t old_value, int16_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

static inline bool
as_cas_uint8(uint8_t* target, uint8_t old_value, uint8_t new_value)
{
	return as_cas_rlx(target, &old_value, new_value);
}

/******************************************************************************
 * MEMORY FENCE
 *****************************************************************************/

// void as_fence_acq()
#define as_fence_acq() __atomic_thread_fence(__ATOMIC_ACQUIRE)

// void as_fence_rls()
#define as_fence_rls() __atomic_thread_fence(__ATOMIC_RELEASE)

// void as_fence_rlx()
// Note - can be used as a compiler barrier, emits no code.
#define as_fence_rlx() __atomic_thread_fence(__ATOMIC_RELAXED)

// void as_fence_seq()
#define as_fence_seq() __atomic_thread_fence(__ATOMIC_SEQ_CST)

// void as_fence_memory()
#define as_fence_memory as_fence_seq

// void as_fence_store()
// Note - no intrinsic equivalent?
#define as_fence_store as_fence_rls

// void as_fence_lock()
#define as_fence_lock as_fence_acq

// void as_fence_unlock()
#define as_fence_unlock as_fence_rls

/******************************************************************************
 * SPIN LOCK
 *****************************************************************************/

typedef ck_spinlock_t as_spinlock;

// void as_spinlock_lock(as_spinlock* lock)
#define as_spinlock_lock ck_spinlock_lock

// void as_spinlock_unlock(as_spinlock* lock)
#define as_spinlock_unlock ck_spinlock_unlock

/******************************************************************************
 * READ/WRITE LOCK
 *****************************************************************************/

typedef ck_swlock_t as_swlock;

// void as_swlock_read_lock(as_swlock* lock)
#define as_swlock_read_lock ck_swlock_read_lock

// void as_swlock_read_unlock(as_swlock* lock)
#define as_swlock_read_unlock ck_swlock_read_unlock

// void as_swlock_write_lock(as_swlock* lock)
#define as_swlock_write_lock ck_swlock_write_lock

// void as_swlock_write_unlock(as_swlock* lock)
#define as_swlock_write_unlock ck_swlock_write_unlock

/******************************************************************************
 * SET MAX
 *****************************************************************************/

static inline bool
as_setmax_int64(int64_t* target, int64_t x)
{
	// Get the current value of the atomic integer.
	int64_t cur = as_load_int64(target);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.

		// Note - can't use as_cas_int64() since we need cur to change.
		if (as_cas_rlx(target, &cur, x)) {
			return true;
		}
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

static inline bool
as_setmax_int32(int32_t* target, int32_t x)
{
	// Get the current value of the atomic integer.
	int32_t cur = as_load_int32(target);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.

		// Note - can't use as_cas_int32() since we need cur to change.
		if (as_cas_rlx(target, &cur, x)) {
			return true;
		}
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

#ifdef __cplusplus
} // end extern "C"
#endif
