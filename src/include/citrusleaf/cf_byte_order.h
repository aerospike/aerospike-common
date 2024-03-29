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

#if defined(__linux__) || defined(__FreeBSD__)

#if defined(__linux__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif

// 16-bit swaps:

static inline uint16_t
cf_swap_to_be16(uint16_t n)
{
	return htobe16(n);
}

static inline uint16_t
cf_swap_to_le16(uint16_t n)
{
	return htole16(n);
}

static inline uint16_t
cf_swap_from_be16(uint16_t n)
{
	return be16toh(n);
}

static inline uint16_t
cf_swap_from_le16(uint16_t n)
{
	return le16toh(n);
}

// 32-bit swaps:

static inline uint32_t
cf_swap_to_be32(uint32_t n)
{
	return htobe32(n);
}

static inline uint32_t
cf_swap_to_le32(uint32_t n)
{
	return htole32(n);
}

static inline uint32_t
cf_swap_from_be32(uint32_t n)
{
	return be32toh(n);
}

static inline uint32_t
cf_swap_from_le32(uint32_t n)
{
	return le32toh(n);
}

// 64-bit swaps:

static inline uint64_t
cf_swap_to_be64(uint64_t n)
{
	return htobe64(n);
}

static inline uint64_t
cf_swap_to_le64(uint64_t n)
{
	return htole64(n);
}

static inline uint64_t
cf_swap_from_be64(uint64_t n)
{
	return be64toh(n);
}

static inline uint64_t
cf_swap_from_le64(uint64_t n)
{
	return le64toh(n);
}

#endif // __linux__ || __FreeBSD__

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#include <arpa/inet.h>

#define cf_swap_to_be16(_n) OSSwapHostToBigInt16(_n)
#define cf_swap_to_le16(_n) OSSwapHostToLittleInt16(_n)
#define cf_swap_from_be16(_n) OSSwapBigToHostInt16(_n)
#define cf_swap_from_le16(_n) OSSwapLittleToHostInt16(_n)

#define cf_swap_to_be32(_n) OSSwapHostToBigInt32(_n)
#define cf_swap_to_le32(_n) OSSwapHostToLittleInt32(_n)
#define cf_swap_from_be32(_n) OSSwapBigToHostInt32(_n)
#define cf_swap_from_le32(_n) OSSwapLittleToHostInt32(_n)

#define cf_swap_to_be64(_n) OSSwapHostToBigInt64(_n)
#define cf_swap_to_le64(_n) OSSwapHostToLittleInt64(_n)
#define cf_swap_from_be64(_n) OSSwapBigToHostInt64(_n)
#define cf_swap_from_le64(_n) OSSwapLittleToHostInt64(_n)
#endif // __APPLE__

#if defined(_MSC_VER)
#include <WinSock2.h>

#define cf_swap_to_be16(_n) _byteswap_ushort(_n)
#define cf_swap_to_le16(_n) (_n)
#define cf_swap_from_be16(_n) _byteswap_ushort(_n)
#define cf_swap_from_le16(_n) (_n)

#define cf_swap_to_be32(_n) _byteswap_ulong(_n)
#define cf_swap_to_le32(_n) (_n)
#define cf_swap_from_be32(_n) _byteswap_ulong(_n)
#define cf_swap_from_le32(_n) (_n)

#define cf_swap_to_be64(_n) _byteswap_uint64(_n)
#define cf_swap_to_le64(_n) (_n)
#define cf_swap_from_be64(_n) _byteswap_uint64(_n)
#define cf_swap_from_le64(_n) (_n)
#endif // _MSC_VER

static inline double
cf_swap_to_big_float64(double d)
{
	uint64_t i = cf_swap_to_be64(*(uint64_t*)&d);
	return *(double*)&i;
}

static inline double
cf_swap_to_little_float64(double d)
{
	uint64_t i = cf_swap_to_le64(*(uint64_t*)&d);
	return *(double*)&i;
}

static inline double
cf_swap_from_big_float64(double d)
{
	uint64_t i = cf_swap_from_be64(*(uint64_t*)&d);
	return *(double*)&i;
}

static inline double
cf_swap_from_little_float64(double d)
{
	uint64_t i = cf_swap_from_le64(*(uint64_t*)&d);
	return *(double*)&i;
}
