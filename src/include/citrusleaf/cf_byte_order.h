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

#define cf_swap_to_be16(_n) htobe16(_n)
#define cf_swap_to_le16(_n) htole16(_n)
#define cf_swap_from_be16(_n) be16toh(_n)
#define cf_swap_from_le16(_n) le16toh(_n)

#define cf_swap_to_be32(_n) htobe32(_n)
#define cf_swap_to_le32(_n) htole32(_n)
#define cf_swap_from_be32(_n) be32toh(_n)
#define cf_swap_from_le32(_n) le32toh(_n)

#define cf_swap_to_be64(_n) htobe64(_n)
#define cf_swap_to_le64(_n) htole64(_n)
#define cf_swap_from_be64(_n) be64toh(_n)
#define cf_swap_from_le64(_n) le64toh(_n)
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

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

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

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
