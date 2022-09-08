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
#include <aerospike/ck/ck_pr.h>
#include <aerospike/ck/ck_spinlock.h>
#include <aerospike/ck/ck_swlock.h>

/******************************************************************************
 * LOAD
 *****************************************************************************/

// void* as_load_ptr(const void** target)
#define as_load_ptr(_target) ck_pr_load_ptr(_target)

// uint64_t as_load_uint64(const uint64_t* target)
#define as_load_uint64(_target) ck_pr_load_64(_target)

// int64_t as_load_int64(const int64_t* target)
#define as_load_int64(_target) (int64_t)ck_pr_load_64((uint64_t*)(_target))

// uint32_t as_load_uint32(const uint32_t* target)
#define as_load_uint32(_target) ck_pr_load_32(_target)

// int32_t as_load_int32(const int32_t* target)
#define as_load_int32(_target) (int32_t)ck_pr_load_32((uint32_t*)(_target))

// uint16_t as_load_uint16(const uint16_t* target)
#define as_load_uint16(_target) ck_pr_load_16(_target)

// int16_t as_load_int16(const int16_t* target)
#define as_load_int16(_target) (int16_t)ck_pr_load_16((uint16_t*)(_target))

// uint8_t as_load_uint8(const uint8_t* target)
#define as_load_uint8(_target) ck_pr_load_8(_target)

// int8_t as_load_int8(const int8_t* target)
#define as_load_int8(_target) (int8_t)ck_pr_load_8((uint8_t*)(_target))

// type as_load_acq(const type* target)
#define as_load_acq(_target) __atomic_load_n(_target, __ATOMIC_ACQUIRE)

// type as_load_rlx(const type* target)
#define as_load_rlx(_target) __atomic_load_n(_target, __ATOMIC_RELAXED)

// type as_load_seq(const type* target)
#define as_load_seq(_target) __atomic_load_n(_target, __ATOMIC_SEQ_CST)

/******************************************************************************
 * STORE
 *****************************************************************************/

// void as_store_ptr(void** target, void* value)
#define as_store_ptr(_target, _value) ck_pr_store_ptr(_target, _value)

// void as_store_uint64(uint64_t* target, uint64_t value)
#define as_store_uint64(_target, _value) ck_pr_store_64(_target, _value)

// void as_store_int64(int64_t* target, int64_t value)
#define as_store_int64(_target, _value) ck_pr_store_64((uint64_t*)(_target), (uint64_t)(_value))

// void as_store_uint32(uint32_t* target, uint32_t value)
#define as_store_uint32(_target, _value) ck_pr_store_32(_target, _value)

// void as_store_int32(uint32_t* target, int32_t value)
#define as_store_int32(_target, _value) ck_pr_store_32((uint32_t*)(_target), (uint32_t)(_value))

// void as_store_uint16(uint16_t* target, uint16_t value)
#define as_store_uint16(_target, _value) ck_pr_store_16(_target, _value)

// void as_store_int16(uint16_t* target, int16_t value)
#define as_store_int16(_target, _value) ck_pr_store_16((uint16_t*)(_target), (uint16_t)(_value))

// void as_store_uint8(uint8_t* target, uint8_t value)
#define as_store_uint8(_target, _value) ck_pr_store_8(_target, _value)

// void as_store_int8(int8_t* target, int8_t value)
#define as_store_int8(_target, _value) ck_pr_store_8((uint8_t*)(_target), (uint8_t)(_value))

// void as_store_rel(type* target, type value)
#define as_store_rel(_target, _value) __atomic_store_n(_target, _value, __ATOMIC_RELEASE)

// void as_store_rlx(type* target, type value)
#define as_store_rlx(_target, _value) __atomic_store_n(_target, _value, __ATOMIC_RELAXED)

// void as_store_seq(type* target, type value)
#define as_store_seq(_target, _value) __atomic_store_n(_target, _value, __ATOMIC_SEQ_CST)

/******************************************************************************
 * INCREMENT
 *****************************************************************************/

// void as_incr_uint64(uint64_t* target)
#define as_incr_uint64(_target) ck_pr_inc_64(_target)

// void as_incr_int64(int64_t* target)
#define as_incr_int64(_target) ck_pr_inc_64((uint64_t*)(_target))

// void as_incr_uint32(uint32_t* target)
#define as_incr_uint32(_target) ck_pr_inc_32(_target)

// void as_incr_int32(int32_t* target)
#define as_incr_int32(_target) ck_pr_inc_32((uint32_t*)(_target))

// void as_incr_uint16(uint16_t* target)
#define as_incr_uint16(_target) ck_pr_inc_16(_target)

// void as_incr_int16(int16_t* target)
#define as_incr_int16(_target) ck_pr_inc_16((uint16_t*)(_target))

/******************************************************************************
 * DECREMENT
 *****************************************************************************/

// void as_decr_uint64(uint64_t* target)
#define as_decr_uint64(_target) ck_pr_dec_64(_target)

// void as_decr_int64(int64_t* target)
#define as_decr_int64(_target) ck_pr_dec_64((uint64_t*)(_target))

// void as_decr_uint32(uint32_t* target)
#define as_decr_uint32(_target) ck_pr_dec_32(_target)

// void as_decr_int32(int32_t* target)
#define as_decr_int32(_target) ck_pr_dec_32((uint32_t*)(_target))

// void as_decr_uint16(uint16_t* target)
#define as_decr_uint16(_target) ck_pr_dec_16(_target)

// void as_decr_int16(int16_t* target)
#define as_decr_int16(_target) ck_pr_dec_16((uint16_t*)(_target))

/******************************************************************************
 * ADD
 *****************************************************************************/

// void as_add_uint64(uint64_t* target, int64_t value)
#define as_add_uint64(_target, _value) ck_pr_add_64(_target, (uint64_t)(_value))

// void as_add_int64(int64_t* target, int64_t value)
#define as_add_int64(_target, _value) ck_pr_add_64((uint64_t*)(_target), (uint64_t)(_value))

// void as_add_uint32(uint32_t* target, int32_t value)
#define as_add_uint32(_target, _value) ck_pr_add_32(_target, (uint32_t)(_value))

// void as_add_int32(int32_t* target, int32_t value)
#define as_add_int32(_target, _value) ck_pr_add_32((uint32_t*)(_target), (uint32_t)(_value))

// void as_add_uint16(uint16_t* target, int16_t value)
#define as_add_uint16(_target, _value) ck_pr_add_16(_target, (uint16_t)(_value))

// void as_add_int16(int16_t* target, int16_t value)
#define as_add_int16(_target, _value) ck_pr_add_16((uint16_t*)(_target), (uint16_t)(_value))

/******************************************************************************
 * FETCH AND ADD
 *****************************************************************************/

// uint64_t as_faa_uint64(uint64_t* target, int64_t value)
#define as_faa_uint64(_target, _value) ck_pr_faa_64(_target, (uint64_t)(_value))

// int64_t as_faa_int64(int64_t* target, int64_t value)
#define as_faa_int64(_target, _value) ck_pr_faa_64((uint64_t*)(_target), (uint64_t)(_value))

// uint32_t as_faa_uint32(uint32_t* target, int32_t value)
#define as_faa_uint32(_target, _value) ck_pr_faa_32(_target, (uint32_t)(_value))

// int32_t as_faa_int32(int32_t* target, int32_t value)
#define as_faa_int32(_target, _value) ck_pr_faa_32((uint32_t*)(_target), (uint32_t)(_value))

// uint16_t as_faa_uint16(uint16_t* target, int16_t value)
#define as_faa_uint16(_target, _value) ck_pr_faa_16(_target, (uint16_t)(_value))

// int16_t as_faa_int16(int16_t* target, int16_t value)
#define as_faa_int16(_target, _value) ck_pr_faa_16((uint16_t*)(_target), (uint16_t)(_value))

// type as_faa_acq(type* target, type value)
#define as_faa_acq(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_ACQUIRE)

// type as_faa_rel(type* target, type value)
#define as_faa_rel(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_RELEASE)

// type as_faa_rlx(type* target, type value)
#define as_faa_rlx(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_RELAXED)

// type as_faa_seq(type* target, type value)
#define as_faa_seq(_target, _value) __atomic_fetch_add(_target, _value, __ATOMIC_SEQ_CST)



/******************************************************************************
 * ADD AND FETCH
 *****************************************************************************/

// uint64_t as_aaf_uint64(uint64_t* target, int64_t value)
#define as_aaf_uint64(_target, _value) (ck_pr_faa_64(_target, (uint64_t)(_value)) + (uint64_t)(_value))

// int64_t as_aaf_int64(int64_t* target, int64_t value)
#define as_aaf_int64(_target, _value) ((int64_t)ck_pr_faa_64((uint64_t*)(_target), (uint64_t)(_value)) + (_value))

// uint32_t as_aaf_uint32(uint32_t* target, int32_t value)
#define as_aaf_uint32(_target, _value) (ck_pr_faa_32(_target, (uint32_t)(_value)) + (uint32_t)(_value))

// int32_t as_aaf_int32(int32_t* target, int32_t value)
#define as_aaf_int32(_target, _value) ((int32_t)ck_pr_faa_32((uint32_t*)(_target), (uint32_t)(_value)) + (_value))

// uint16_t as_aaf_uint16(uint16_t* target, int16_t value)
#define as_aaf_uint16(_target, _value) ((uint16_t)(ck_pr_faa_16(_target, (uint16_t)(_value)) + (_value)))

// int16_t as_aaf_int16(int16_t* target, int16_t value)
#define as_aaf_int16(_target, _value) ((int16_t)(ck_pr_faa_16((uint16_t*)(_target), (uint16_t)(_value)) + (_value)))

// type as_aaf_acq(type* target, type value)
#define as_aaf_acq(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_ACQUIRE)

// type as_aaf_rel(type* target, type value)
#define as_aaf_rel(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_RELEASE)

// type as_aaf_rlx(type* target, type value)
#define as_aaf_rlx(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_RELAXED)

// type as_aaf_seq(type* target, type value)
#define as_aaf_seq(_target, _value) __atomic_add_fetch(_target, _value, __ATOMIC_SEQ_CST)

/******************************************************************************
 * FETCH AND SWAP
 *****************************************************************************/

// uint64_t as_fas_uint64(uint64_t* target, uint64_t value)
#define as_fas_uint64(_target, _value) ck_pr_fas_64(_target, _value)

// int64_t as_fas_int64(int64_t* target, int64_t value)
#define as_fas_int64(_target, _value) (int64_t)ck_pr_fas_64((uint64_t*)(_target), (uint64_t)(_value))

// uint32_t as_fas_uint32(uint32_t* target, uint32_t value)
#define as_fas_uint32(_target, _value) ck_pr_fas_32(_target, _value)

// int32_t as_fas_int32(int32_t* target, int32_t value)
#define as_fas_int32(_target, _value) (int32_t)ck_pr_fas_32((uint32_t*)(_target), (uint32_t)(_value))

// uint16_t as_fas_uint16(uint16_t* target, uint16_t value)
#define as_fas_uint16(_target, _value) ck_pr_fas_16(_target, _value)

// int16_t as_fas_int16(int16_t* target, int16_t value)
#define as_fas_int16(_target, _value) (int16_t)ck_pr_fas_16((uint16_t*)(_target), (uint16_t)(_value))

// type as_fas_acq(type* target, type value)
#define as_fas_acq(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_ACQUIRE)

// type as_fas_rel(type* target, type value)
#define as_fas_rel(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_RELEASE)

// type as_fas_rlx(type* target, type value)
#define as_fas_rlx(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_RELAXED)

// type as_fas_seq(type* target, type value)
#define as_fas_seq(_target, _value) __atomic_exchange_n(_target, _value, __ATOMIC_SEQ_CST)

/******************************************************************************
 * COMPARE AND SWAP
 *****************************************************************************/

// bool as_cas_uint64(uint64_t* target, uint64_t old_value, uint64_t new_value)
#define as_cas_uint64(_target, _old_value, _new_value) ck_pr_cas_64(_target, _old_value, _new_value)

// bool as_cas_int64(int64_t* target, int64_t old_value, int64_t new_value)
#define as_cas_int64(_target, _old_value, _new_value) ck_pr_cas_64((uint64_t*)(_target), (uint64_t)(_old_value), (uint64_t)(_new_value))

// bool as_cas_uint32(uint32_t* target, uint32_t old_value, uint32_t new_value)
#define as_cas_uint32(_target, _old_value, _new_value) ck_pr_cas_32(_target, _old_value, _new_value)

// bool as_cas_int32(int32_t* target, int32_t old_value, int32_t new_value)
#define as_cas_int32(_target, _old_value, _new_value) ck_pr_cas_32((uint32_t*)(_target), (uint32_t)(_old_value), (uint32_t)(_new_value))

// bool as_cas_uint16(uint16_t* target, uint16_t old_value, uint16_t new_value)
#define as_cas_uint16(_target, _old_value, _new_value) ck_pr_cas_16(_target, _old_value, _new_value)

// bool as_cas_int16(int16_t* target, int16_t old_value, int16_t new_value)
#define as_cas_int16(_target, _old_value, _new_value) ck_pr_cas_16((uint16_t*)(_target), (uint16_t)(_old_value), (uint16_t)(_new_value))

// bool as_cas_uint8(uint8_t* target, uint8_t old_value, uint8_t new_value)
#define as_cas_uint8(_target, _old_value, _new_value) ck_pr_cas_8(_target, _old_value, _new_value)

// bool as_cas_acq(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_acq(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

// bool as_cas_rel(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_rel(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)

// bool as_cas_rlx(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_rlx(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

// bool as_cas_seq(type* target, type* old_value, type new_value)
// Note - old_value is returned by pointer.
#define as_cas_seq(_target, _old_value, _new_value) __atomic_compare_exchange_n(_target, _old_value, _new_value, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)

/******************************************************************************
 * MEMORY FENCE
 *****************************************************************************/

// void as_fence_memory()
#define as_fence_memory ck_pr_fence_memory

// void as_fence_store()
#define as_fence_store ck_pr_fence_store

// void as_fence_lock()
#define as_fence_lock ck_pr_fence_lock

// void as_fence_unlock()
#define as_fence_unlock ck_pr_fence_unlock

// void as_fence_acq()
#define as_fence_acq() __atomic_thread_fence(__ATOMIC_ACQUIRE)

// void as_fence_rel()
#define as_fence_rel() __atomic_thread_fence(__ATOMIC_RELEASE)

// void as_fence_rlx()
// Note - can be used as a compiler barrier, emits no code.
#define as_fence_rlx() __atomic_thread_fence(__ATOMIC_RELAXED)

// void as_fence_seq()
#define as_fence_seq() __atomic_thread_fence(__ATOMIC_SEQ_CST)

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
	uint64_t prior;

	// Get the current value of the atomic integer.
	int64_t cur = as_load_int64(target);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		if (ck_pr_cas_64_value((uint64_t*)target, (uint64_t)cur, (uint64_t)x,
				&prior)) {
			// Current value was unchanged, proposed value swapped in.
			return true;
		}

		// Current value had changed, set cur to prior and go around again.
		cur = (int64_t)prior;
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

static inline bool
as_setmax_int32(int32_t* target, int32_t x)
{
	uint32_t prior;

	// Get the current value of the atomic integer.
	int32_t cur = as_load_int32(target);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		if (ck_pr_cas_32_value((uint32_t*)target, (uint32_t)cur, (uint32_t)x,
				&prior)) {
			return true;
		}

		// Current value had changed, set cur to prior and go around again.
		cur = (int32_t)prior;
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

#ifdef __cplusplus
} // end extern "C"
#endif
