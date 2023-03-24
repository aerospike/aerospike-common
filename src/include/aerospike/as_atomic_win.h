/*
 * Copyright 2008-2023 Aerospike, Inc.
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

// Atomics for Windows
#include <aerospike/as_std.h>

#if defined(WIN32_LEAN_AND_MEAN)
#include <windows.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#include <intrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// We assume for now that our Windows clients will be run only on x86. The
// wrappers with "acquire" and "release" barrier semantics do not actually
// enforce the barriers - the wrappers are there to enable compilation only.

// TODO - if we find we need the Windows client to run on ARM, revisit the
// acquire and release wrapped methods and implement them correctly.

/******************************************************************************
 * LOAD
 *****************************************************************************/

// void* as_load_ptr(const void** target)
#define as_load_ptr(_target) (*(void volatile**)(_target))

// uint64_t as_load_uint64(const uint64_t* target)
#define as_load_uint64(_target) (*(uint64_t volatile*)(_target))

// int64_t as_load_int64(const int64_t* target)
#define as_load_int64(_target) (*(int64_t volatile*)(_target))

// uint32_t as_load_uint32(const uint32_t* target)
#define as_load_uint32(_target) (*(uint32_t volatile*)(_target))

// int32_t as_load_int32(const int32_t* target)
#define as_load_int32(_target) (*(int32_t volatile*)(_target))

// uint16_t as_load_uint16(const uint16_t* target)
#define as_load_uint16(_target) (*(uint16_t volatile*)(_target))

// int16_t as_load_int16(const int16_t* target)
#define as_load_int16(_target) (*(int16_t volatile*)(_target))

// uint8_t as_load_uint8(const uint8_t* target)
#define as_load_uint8(_target) (*(uint8_t volatile*)(_target))

// int8_t as_load_int8(const int8_t* target)
#define as_load_int8(_target) (*(int8_t volatile*)(_target))

// Assume Windows clients run only on x86 - wrappers to enable compilation only.

// void* as_load_ptr_acq(const void** target)
#define as_load_ptr_acq(_target) (*(void volatile**)(_target))

// uint64_t as_load_uint64_acq(const uint64_t* target)
#define as_load_uint64_acq(_target) (*(uint64_t volatile*)(_target))

// int64_t as_load_int64_acq(const int64_t* target)
#define as_load_int64_acq(_target) (*(int64_t volatile*)(_target))

// uint32_t as_load_uint32_acq(const uint32_t* target)
#define as_load_uint32_acq(_target) (*(uint32_t volatile*)(_target))

// int32_t as_load_int32_acq(const int32_t* target)
#define as_load_int32_acq(_target) (*(int32_t volatile*)(_target))

// uint16_t as_load_uint16_acq(const uint16_t* target)
#define as_load_uint16_acq(_target) (*(uint16_t volatile*)(_target))

// int16_t as_load_int16_acq(const int16_t* target)
#define as_load_int16_acq(_target) (*(int16_t volatile*)(_target))

// uint8_t as_load_uint8_acq(const uint8_t* target)
#define as_load_uint8_acq(_target) (*(uint8_t volatile*)(_target))

// int8_t as_load_int8_acq(const int8_t* target)
#define as_load_int8_acq(_target) (*(int8_t volatile*)(_target))

/******************************************************************************
 * STORE
 *****************************************************************************/

// void as_store_ptr(void** target, void* value)
#define as_store_ptr(_target, _value) *(void volatile**)(_target) = _value

// void as_store_uint64(uint64_t* target, uint64_t value)
#define as_store_uint64(_target, _value) *(uint64_t volatile*)(_target) = _value

// void as_store_int64(int64_t* target, int64_t value)
#define as_store_int64(_target, _value) *(int64_t volatile*)(_target) = _value

// void as_store_uint32(uint32_t* target, uint32_t value)
#define as_store_uint32(_target, _value) *(uint32_t volatile*)(_target) = _value

// void as_store_int32(uint32_t* target, int32_t value)
#define as_store_int32(_target, _value) *(int32_t volatile*)(_target) = _value

// void as_store_uint16(uint16_t* target, uint16_t value)
#define as_store_uint16(_target, _value) *(uint16_t volatile*)(_target) = _value

// void as_store_int16(uint16_t* target, int16_t value)
#define as_store_int16(_target, _value) *(int16_t volatile*)(_target) = _value

// void as_store_uint8(uint8_t* target, uint8_t value)
#define as_store_uint8(_target, _value) *(uint8_t volatile*)(_target) = _value

// void as_store_int8(int8_t* target, int8_t value)
#define as_store_int8(_target, _value) *(int8_t volatile*)(_target) = _value

// Assume Windows clients run only on x86 - wrappers to enable compilation only.

// void as_store_ptr_rls(void** target, void* value)
#define as_store_ptr_rls(_target, _value) *(void volatile**)(_target) = _value

// void as_store_uint64_rls(uint64_t* target, uint64_t value)
#define as_store_uint64_rls(_target, _value) *(uint64_t volatile*)(_target) = _value

// void as_store_int64_rls(int64_t* target, int64_t value)
#define as_store_int64_rls(_target, _value) *(int64_t volatile*)(_target) = _value

// void as_store_uint32_rls(uint32_t* target, uint32_t value)
#define as_store_uint32_rls(_target, _value) *(uint32_t volatile*)(_target) = _value

// void as_store_int32_rls(uint32_t* target, int32_t value)
#define as_store_int32_rls(_target, _value) *(int32_t volatile*)(_target) = _value

// void as_store_uint16_rls(uint16_t* target, uint16_t value)
#define as_store_uint16_rls(_target, _value) *(uint16_t volatile*)(_target) = _value

// void as_store_int16_rls(uint16_t* target, int16_t value)
#define as_store_int16_rls(_target, _value) *(int16_t volatile*)(_target) = _value

// void as_store_uint8_rls(uint8_t* target, uint8_t value)
#define as_store_uint8_rls(_target, _value) *(uint8_t volatile*)(_target) = _value

// void as_store_int8_rls(int8_t* target, int8_t value)
#define as_store_int8_rls(_target, _value) *(int8_t volatile*)(_target) = _value

/******************************************************************************
 * FETCH AND ADD
 *****************************************************************************/

// uint64_t as_faa_uint64(uint64_t* target, int64_t value)
#define as_faa_uint64(_target, _value) (uint64_t)InterlockedExchangeAdd64((LONGLONG volatile*)(_target), _value)

// int64_t as_faa_int64(int64_t* target, int64_t value)
#define as_faa_int64(_target, _value) InterlockedExchangeAdd64((LONGLONG volatile*)(_target), _value)

// uint32_t as_faa_uint32(uint32_t* target, int32_t value)
#define as_faa_uint32(_target, _value) (uint32_t)InterlockedExchangeAdd((LONG volatile*)(_target), _value)

// int32_t as_faa_int32(int32_t* target, int32_t value)
#define as_faa_int32(_target, _value) InterlockedExchangeAdd((LONG volatile*)(_target), _value)

// Windows does not support 16 bit atomic fetch/add.
// uint16_t as_faa_uint16(uint16_t* target, int16_t value)
// int16_t as_faa_int16(int16_t* target, int16_t value)

/******************************************************************************
 * ADD AND FETCH
 *****************************************************************************/

// uint64_t as_aaf_uint64(uint64_t* target, int64_t value)
#define as_aaf_uint64(_target, _value) (uint64_t)InterlockedAdd64((LONGLONG volatile*)(_target), _value)

// int64_t as_aaf_int64(int64_t* target, int64_t value)
#define as_aaf_int64(_target, _value) InterlockedAdd64((LONGLONG volatile*)(_target), _value)

// uint32_t as_aaf_uint32(uint32_t* target, int32_t value)
#define as_aaf_uint32(_target, _value) (uint32_t)InterlockedAdd((LONG volatile*)(_target), _value)

// int32_t as_aaf_int32(int32_t* target, int32_t value)
#define as_aaf_int32(_target, _value) InterlockedAdd((LONG volatile*)(_target), _value)

// Assume Windows clients run only on x86 - wrappers to enable compilation only.

// uint64_t as_aaf_uint64_rls(uint64_t* target, int64_t value)
#define as_aaf_uint64_rls(_target, _value) (uint64_t)InterlockedAdd64((LONGLONG volatile*)(_target), _value)

// int64_t as_aaf_int64_rls(int64_t* target, int64_t value)
#define as_aaf_int64_rls(_target, _value) InterlockedAdd64((LONGLONG volatile*)(_target), _value)

// uint32_t as_aaf_uint32_rls(uint32_t* target, int32_t value)
#define as_aaf_uint32_rls(_target, _value) (uint32_t)InterlockedAdd((LONG volatile*)(_target), _value)

// int32_t as_aaf_int32_rls(int32_t* target, int32_t value)
#define as_aaf_int32_rls(_target, _value) InterlockedAdd((LONG volatile*)(_target), _value)

// Windows does not support 16 bit atomic add/fetch.
// uint16_t as_aaf_uint16(uint16_t* target, int16_t value)
// int16_t as_aaf_int16(int16_t* target, int16_t value)
// uint16_t as_aaf_uint16_rls(uint16_t* target, int16_t value)
// int16_t as_aaf_int16_rls(int16_t* target, int16_t value)

/******************************************************************************
 * ADD
 *****************************************************************************/

// void as_add_uint64(uint64_t* target, int64_t value)
#define as_add_uint64(_target, _value) InterlockedExchangeAdd64((LONGLONG volatile*)(_target), _value)

// void as_add_int64(int64_t* target, int64_t value)
#define as_add_int64(_target, _value) InterlockedExchangeAdd64((LONGLONG volatile*)(_target), _value)

// void as_add_uint32(uint32_t* target, int32_t value)
#define as_add_uint32(_target, _value) InterlockedExchangeAdd((LONG volatile*)(_target), _value)

// void as_add_int32(int32_t* target, int32_t value)
#define as_add_int32(_target, _value) InterlockedExchangeAdd((LONG volatile*)(_target), _value)

// Windows does not support 16 bit atomic add.
// void as_add_uint16(uint16_t* target, int16_t value)
// void as_add_int16(int16_t* target, int16_t value)

/******************************************************************************
 * INCREMENT
 *****************************************************************************/

// void as_incr_uint64(uint64_t* target)
#define as_incr_uint64(_target) InterlockedIncrement64((LONGLONG volatile*)(_target))

// void as_incr_int64(int64_t* target)
#define as_incr_int64(_target) InterlockedIncrement64((LONGLONG volatile*)(_target))

// void as_incr_uint32(uint32_t* target)
#define as_incr_uint32(_target) InterlockedIncrement((LONG volatile*)(_target))

// void as_incr_int32(int32_t* target)
#define as_incr_int32(_target) InterlockedIncrement((LONG volatile*)(_target))

// void as_incr_uint16(uint16_t* target)
#define as_incr_uint16(_target) InterlockedIncrement16((short volatile*)(_target))

// void as_incr_int16(int16_t* target)
#define as_incr_int16(_target) InterlockedIncrement16((short volatile*)(_target))

// Assume Windows clients run only on x86 - wrappers to enable compilation only.

// void as_incr_uint64_rls(uint64_t* target)
#define as_incr_uint64_rls(_target) InterlockedIncrement64((LONGLONG volatile*)(_target))

// void as_incr_int64_rls(int64_t* target)
#define as_incr_int64_rls(_target) InterlockedIncrement64((LONGLONG volatile*)(_target))

// void as_incr_uint32_rls(uint32_t* target)
#define as_incr_uint32_rls(_target) InterlockedIncrement((LONG volatile*)(_target))

// void as_incr_int32_rls(int32_t* target)
#define as_incr_int32_rls(_target) InterlockedIncrement((LONG volatile*)(_target))

// void as_incr_uint16_rls(uint16_t* target)
#define as_incr_uint16_rls(_target) InterlockedIncrement16((short volatile*)(_target))

// void as_incr_int16_rls(int16_t* target)
#define as_incr_int16_rls(_target) InterlockedIncrement16((short volatile*)(_target))

/******************************************************************************
 * DECREMENT
 *****************************************************************************/

// void as_decr_uint64(uint64_t* target)
#define as_decr_uint64(_target) InterlockedDecrement64((LONGLONG volatile*)(_target))

// void as_decr_int64(int64_t* target)
#define as_decr_int64(_target) InterlockedDecrement64((LONGLONG volatile*)(_target))

// void as_decr_uint32(uint32_t* target)
#define as_decr_uint32(_target) InterlockedDecrement((LONG volatile*)(_target))

// void as_decr_int32(int32_t* target)
#define as_decr_int32(_target) InterlockedDecrement((LONG volatile*)(_target))

// void as_decr_uint16(uint16_t* target)
#define as_decr_uint16(_target) InterlockedDecrement16((short volatile*)(_target))

// void as_decr_int16(int16_t* target)
#define as_decr_int16(_target) InterlockedDecrement16((short volatile*)(_target))

// Assume Windows clients run only on x86 - wrappers to enable compilation only.

// void as_decr_uint64_rls(uint64_t* target)
#define as_decr_uint64_rls(_target) InterlockedDecrement64((LONGLONG volatile*)(_target))

// void as_decr_int64_rls(int64_t* target)
#define as_decr_int64_rls(_target) InterlockedDecrement64((LONGLONG volatile*)(_target))

// void as_decr_uint32_rls(uint32_t* target)
#define as_decr_uint32_rls(_target) InterlockedDecrement((LONG volatile*)(_target))

// void as_decr_int32_rls(int32_t* target)
#define as_decr_int32_rls(_target) InterlockedDecrement((LONG volatile*)(_target))

// void as_decr_uint16_rls(uint16_t* target)
#define as_decr_uint16_rls(_target) InterlockedDecrement16((short volatile*)(_target))

// void as_decr_int16_rls(int16_t* target)
#define as_decr_int16_rls(_target) InterlockedDecrement16((short volatile*)(_target))

/******************************************************************************
 * FETCH AND SWAP
 *****************************************************************************/

// uint64_t as_fas_uint64(uint64_t* target, uint64_t value)
#define as_fas_uint64(_target, _value) (uint64_t)InterlockedExchange64((LONGLONG volatile*)(_target), (LONGLONG)(_value))

// int64_t as_fas_int64(int64_t* target, int64_t value)
#define as_fas_int64(_target, _value) InterlockedExchange64((LONGLONG volatile*)(_target), (LONGLONG)(_value))

// uint32_t as_fas_uint32(uint32_t* target, uint32_t value)
#define as_fas_uint32(_target, _value) (uint32_t)InterlockedExchange((LONG volatile*)(_target), (LONG)(_value))

// int32_t as_fas_int32(int32_t* target, int32_t value)
#define as_fas_int32(_target, _value) InterlockedExchange((LONG volatile*)(_target), (LONG)(_value))

// uint16_t as_fas_uint16(uint16_t* target, uint16_t value)
#define as_fas_uint16(_target, _value) (uint16_t)InterlockedExchange16((short volatile*)(_target), (LONG)(_value))

// int16_t as_fas_int16(int16_t* target, int16_t value)
#define as_fas_int16(_target, _value) InterlockedExchange16((short volatile*)(_target), (LONG)(_value))

/******************************************************************************
 * COMPARE AND SWAP
 *****************************************************************************/

// bool as_cas_uint64(uint64_t* target, uint64_t old_value, uint64_t new_value)
#define as_cas_uint64(_target, _old_value, _new_value) (InterlockedCompareExchange64((LONGLONG volatile*)(_target), (LONGLONG)(_new_value), (LONGLONG)(_old_value)) == (LONGLONG)(_old_value))

// bool as_cas_int64(int64_t* target, int64_t old_value, int64_t new_value)
#define as_cas_int64(_target, _old_value, _new_value) (InterlockedCompareExchange64((LONGLONG volatile*)(_target), (LONGLONG)(_new_value), (LONGLONG)(_old_value)) == (LONGLONG)(_old_value))

// bool as_cas_uint32(uint32_t* target, uint32_t old_value, uint32_t new_value)
#define as_cas_uint32(_target, _old_value, _new_value) (InterlockedCompareExchange((LONG volatile*)(_target), (LONG)(_new_value), (LONG)(_old_value)) == (LONG)(_old_value))

// bool as_cas_int32(int32_t* target, int32_t old_value, int32_t new_value)
#define as_cas_int32(_target, _old_value, _new_value) (InterlockedCompareExchange((LONG volatile*)(_target), (LONG)(_new_value), (LONG)(_old_value)) == (LONG)(_old_value))

// bool as_cas_uint16(uint16_t* target, uint16_t old_value, uint16_t new_value)
#define as_cas_uint16(_target, _old_value, _new_value) (InterlockedCompareExchange16((short volatile*)(_target), (short)(_new_value), (short)(_old_value)) == (short)(_old_value))

// bool as_cas_int16(int16_t* target, int16_t old_value, int16_t new_value)
#define as_cas_int16(_target, _old_value, _new_value) (InterlockedCompareExchange16((short volatile*)(_target), (short)(_new_value), (short)(_old_value)) == (short)(_old_value))

// bool as_cas_uint8(uint8_t* target, uint8_t old_value, uint8_t new_value)
#define as_cas_uint8(_target, _old_value, _new_value) (_InterlockedCompareExchange8((char volatile*)(_target), (char)(_new_value), (char)(_old_value)) == (char)(_old_value))

// bool as_cas_int8(int8_t* target, int8_t old_value, int8_t new_value)
#define as_cas_int8(_target, _old_value, _new_value) (_InterlockedCompareExchange8((char volatile*)(_target), (char)(_new_value), (char)(_old_value)) == (char)(_old_value))

/******************************************************************************
 * MEMORY FENCE
 *****************************************************************************/

// The atomic include causes compiler errors in Visual Studio 17. Therefore,
// the new atomic_thread_fence() memory barriers can't be used yet.
// #include <atomic>

// void as_fence_acq()
#define as_fence_acq MemoryBarrier

// void as_fence_rls()
#define as_fence_rls MemoryBarrier

// void as_fence_rlx()
#define as_fence_rlx MemoryBarrier

// void as_fence_seq()
#define as_fence_seq MemoryBarrier

/******************************************************************************
 * SPIN LOCK
 *****************************************************************************/

typedef uint32_t as_spinlock;

#define AS_SPINLOCK_INIT { 0 }
#define as_spinlock_init(_s) *(_s) = 0
#define as_spinlock_destroy(_s) ((void)_s) // no-op

static inline void
as_spinlock_lock(as_spinlock* lock)
{
	while (as_fas_uint32(lock, 1) == 1) {
		while (as_load_uint32(lock) == 1)
			YieldProcessor();
	}

	MemoryBarrier();
}

static inline void
as_spinlock_unlock(as_spinlock* lock)
{
	MemoryBarrier();
	as_store_uint32(lock, 0);
}

/******************************************************************************
 * SPIN WRITER/READERS LOCK
 *****************************************************************************/

typedef uint32_t as_swlock;

#define AS_SWLOCK_INIT { 0 }
#define as_swlock_init(_s) (_s) = 0
#define as_swlock_destroy(_s) ((void)_s) // no-op

#define AS_SWLOCK_WRITER_BIT (1 << 31)
#define AS_SWLOCK_LATCH_BIT (1 << 30)
#define AS_SWLOCK_WRITER_MASK (AS_SWLOCK_LATCH_BIT | AS_SWLOCK_WRITER_BIT)
#define AS_SWLOCK_READER_MASK (UINT32_MAX ^ AS_SWLOCK_WRITER_MASK)

static inline void
as_swlock_write_lock(as_swlock* lock)
{
	InterlockedOr((LONG volatile*)lock, AS_SWLOCK_WRITER_BIT);

	while ((as_load_uint32(lock) & AS_SWLOCK_READER_MASK) != 0) {
		YieldProcessor();
	}

	MemoryBarrier();
}

static inline void
as_swlock_write_unlock(as_swlock* lock)
{
	MemoryBarrier();
	InterlockedAnd((LONG volatile*)lock, AS_SWLOCK_READER_MASK);
}

static inline void
as_swlock_read_lock(as_swlock* lock)
{
	while (true) {
		while ((as_load_uint32(lock) & AS_SWLOCK_WRITER_BIT) != 0) {
			YieldProcessor();
		}

		uint32_t l = as_faa_uint32(lock, 1) & AS_SWLOCK_WRITER_MASK;

		if (l == 0) {
			break;
		}

		if (l == AS_SWLOCK_WRITER_BIT) {
			as_decr_uint32(lock);
		}
	}

	MemoryBarrier();
}

static inline void
as_swlock_read_unlock(as_swlock* lock)
{
	MemoryBarrier();
	as_decr_uint32(lock);
}

/******************************************************************************
 * SET MAX
 *****************************************************************************/

static inline bool
as_setmax_uint64(uint64_t* target, uint64_t x)
{
	uint64_t prior;

	// Get the current value of the atomic integer.
	uint64_t cur = as_load_uint64(target);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		prior = InterlockedCompareExchange64((LONGLONG volatile*)target, x, cur);

		if (cur == prior) {
			// Current value was unchanged, proposed value swapped in.
			return true;
		}

		// Current value had changed, set cur to prior and go around again.
		cur = prior;
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

static inline bool
as_setmax_uint32(uint32_t* target, uint32_t x)
{
	uint32_t prior;

	// Get the current value of the atomic integer.
	uint32_t cur = as_load_uint32(target);

	while (x > cur) {
		// Proposed value is larger than current - attempt compare-and-swap.
		prior = InterlockedCompareExchange((LONG volatile*)target, x, cur);

		if (cur == prior) {
			return true;
		}

		// Current value had changed, set cur to prior and go around again.
		cur = prior;
	}

	// Proposed value not swapped in as new maximum.
	return false;
}

#ifdef __cplusplus
} // end extern "C"
#endif
