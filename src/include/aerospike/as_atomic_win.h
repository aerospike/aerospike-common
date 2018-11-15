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
/*
 * Copyright 2014 Jaidev Sridhar.
 * Copyright 2014 Samy Al Bahra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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

/******************************************************************************
 * MEMORY FENCE
 *****************************************************************************/

// void as_fence_memory()
#define as_fence_memory MemoryBarrier

// void as_fence_store()
#define as_fence_store _WriteBarrier

// void as_fence_lock()
#define as_fence_lock MemoryBarrier

// void as_fence_unlock()
#define as_fence_unlock MemoryBarrier

/******************************************************************************
 * SPIN LOCK
 *****************************************************************************/

typedef uint32_t as_spinlock;

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
 * READ/WRITE LOCK
 *****************************************************************************/

#define AS_SWLOCK_INITIALIZER {0}
#define AS_SWLOCK_WRITER_BIT (1UL << 31)
#define AS_SWLOCK_LATCH_BIT	(1UL << 30)
#define AS_SWLOCK_WRITER_MASK (AS_SWLOCK_LATCH_BIT | AS_SWLOCK_WRITER_BIT)
#define AS_SWLOCK_READER_MASK (UINT32_MAX ^ AS_SWLOCK_WRITER_MASK)

typedef uint32_t as_swlock;

static inline void
as_swlock_read_lock(as_swlock* lock)
{
	uint32_t l;

	for (;;) {
		while (as_load_uint32(lock) & AS_SWLOCK_WRITER_BIT)
			YieldProcessor();

		l = as_faa_uint32(lock, 1) & AS_SWLOCK_WRITER_MASK;
		if (l == 0)
			break;

		if (l == AS_SWLOCK_WRITER_BIT)
			as_decr_uint32(lock);
	}

	MemoryBarrier();
}

static inline void
as_swlock_read_unlock(as_swlock* lock)
{
	MemoryBarrier();
	as_decr_uint32(lock);
}

static inline void
as_swlock_write_lock(as_swlock* lock)
{
	InterlockedOr((LONG volatile*)lock, AS_SWLOCK_WRITER_BIT);
	while (as_load_uint32(lock) & AS_SWLOCK_READER_MASK)
		YieldProcessor();

	MemoryBarrier();
}

static inline void
as_swlock_write_unlock(as_swlock* lock)
{
	MemoryBarrier();
	InterlockedAnd((LONG volatile*)lock, AS_SWLOCK_READER_MASK);
	return;
}

/******************************************************************************
 * SET MAX
 *****************************************************************************/

static inline bool
as_setmax_int64(int64_t* target, int64_t x)
{
	int64_t prior;

	// Get the current value of the atomic integer.
	int64_t cur = as_load_int64(target);

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
as_setmax_int32(int32_t* target, int32_t x)
{
	int32_t prior;

	// Get the current value of the atomic integer.
	int32_t cur = as_load_int32(target);

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
