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

//==========================================================
// Includes.
//

#include <aerospike/as_std.h>
#include <stddef.h>

#include "cf_byte_order.h"

#ifdef __cplusplus
extern "C" {
#endif


//==========================================================
// Public API.
//

// 32-bit Fowler-Noll-Vo hash function (FNV-1a).
static inline uint32_t
cf_hash_fnv32(const uint8_t* buf, size_t size)
{
	uint32_t hash = 2166136261;
	const uint8_t* end = buf + size;

	while (buf < end) {
		hash ^= (uint32_t)*buf++;
		hash *= 16777619;
	}

	return hash;
}

// 64-bit Fowler-Noll-Vo hash function (FNV-1a).
static inline uint64_t
cf_hash_fnv64(const uint8_t* buf, size_t size)
{
	uint64_t hash = 0xcbf29ce484222325ULL;
	const uint8_t* end = buf + size;

	while (buf < end) {
		hash ^= (uint64_t)*buf++;
		hash *= 0x100000001b3ULL;
	}

	return hash;
}

// 32-bit Jenkins One-at-a-Time hash function.
static inline uint32_t
cf_hash_jen32(const uint8_t* buf, size_t size)
{
	uint32_t hash = 0;
	const uint8_t* end = buf + size;

	while (buf < end) {
		hash += (uint32_t)*buf++;
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash;
}

// 64-bit Jenkins One-at-a-Time hash function.
static inline uint64_t
cf_hash_jen64(const uint8_t* buf, size_t size)
{
	uint64_t hash = 0;
	const uint8_t* end = buf + size;

	while (buf < end) {
		hash += (uint64_t)*buf++;
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash;
}

// 32-bit pointer hash.
static inline uint32_t
cf_hash_ptr32(const void* p_ptr)
{
	return (uint32_t)((*(const uint64_t*)p_ptr * 0xe221f97c30e94e1dULL) >> 32);
}

//------------------------------------------------
// murmurhash3_x64_128
//

#define ROTL(x, r) ((x << r) | (x >> (64 - r)))

static inline uint64_t
fmix64(uint64_t k)
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccd;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53;
	k ^= k >> 33;

	return k;
}

// Public API.
static inline void
murmurHash3_x64_128(const uint8_t* key, const size_t len, uint8_t* out)
{
	const uint8_t* data = (const uint8_t*)key;
	const size_t nblocks = len / 16;

	uint64_t h1 = 0;
	uint64_t h2 = 0;

	const uint64_t c1 = 0x87c37b91114253d5;
	const uint64_t c2 = 0x4cf5ad432745937f;

	//----------
	// body

	const uint64_t* blocks = (const uint64_t*)(data);

	for (size_t i = 0; i < nblocks; i++) {
		uint64_t k1 = cf_swap_from_le64(blocks[i * 2 + 0]);
		uint64_t k2 = cf_swap_from_le64(blocks[i * 2 + 1]);

		k1 *= c1;
		k1 = ROTL(k1, 31);
		k1 *= c2; h1 ^= k1;

		h1 = ROTL(h1, 27);
		h1 += h2;
		h1 = h1 * 5+0x52dce729;

		k2 *= c2;
		k2 = ROTL(k2, 33);
		k2 *= c1; h2 ^= k2;

		h2 = ROTL(h2, 31);
		h2 += h1;
		h2 = h2 * 5+0x38495ab5;
	}

	//----------
	// tail

	const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);

	uint64_t k1 = 0;
	uint64_t k2 = 0;

	switch (len & 15) {
	case 15:
		k2 ^= ((uint64_t)tail[14]) << 48;
		// No break.
	case 14:
		k2 ^= ((uint64_t)tail[13]) << 40;
		// No break.
	case 13:
		k2 ^= ((uint64_t)tail[12]) << 32;
		// No break.
	case 12:
		k2 ^= ((uint64_t)tail[11]) << 24;
		// No break.
	case 11:
		k2 ^= ((uint64_t)tail[10]) << 16;
		// No break.
	case 10:
		k2 ^= ((uint64_t)tail[9]) << 8;
		// No break.
	case  9:
		k2 ^= ((uint64_t)tail[8]) << 0;

		k2 *= c2;
		k2 = ROTL(k2, 33);
		k2 *= c1;
		h2 ^= k2;

		// No break.
	case  8:
		k1 ^= ((uint64_t)tail[7]) << 56;
		// No break.
	case  7:
		k1 ^= ((uint64_t)tail[6]) << 48;
		// No break.
	case  6:
		k1 ^= ((uint64_t)tail[5]) << 40;
		// No break.
	case  5:
		k1 ^= ((uint64_t)tail[4]) << 32;
		// No break.
	case  4:
		k1 ^= ((uint64_t)tail[3]) << 24;
		// No break.
	case  3:
		k1 ^= ((uint64_t)tail[2]) << 16;
		// No break.
	case  2:
		k1 ^= ((uint64_t)tail[1]) << 8;
		// No break.
	case  1:
		k1 ^= ((uint64_t)tail[0]) << 0;

		k1 *= c1;
		k1 = ROTL(k1, 31);
		k1 *= c2;
		h1 ^= k1;
	}

	//----------
	// finalization

	h1 ^= len;
	h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64(h1);
	h2 = fmix64(h2);

	h1 += h2;
	h2 += h1;

	((uint64_t*)out)[0] = h1;
	((uint64_t*)out)[1] = h2;
}

//------------------------------------------------
// wyhash
//

#if ! defined(__SIZEOF_INT128__) && defined(_MSC_VER) && defined(_M_X64)
// Microsoft, has 128 bit native type, no combining.
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif

static inline void
cf_mult128(uint64_t* A, uint64_t* B)
{
#if defined(__SIZEOF_INT128__)
	// Safe for gcc, clang with ARM and X86_64.
	__uint128_t r = *A;
	r *= *B;
	*A = (uint64_t)r;
	*B = (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
	*A = _umul128(*A, *B, B);
#else
	// Very much reduced performance!
	#warning "128-bit multiplication by hand"
	uint64_t ha = *A >> 32;
	uint64_t hb = *B >> 32;
	uint64_t la = (uint32_t)*A;
	uint64_t lb = (uint32_t)*B;
	uint64_t rh = ha * hb;
	uint64_t rm0 = ha * lb;
	uint64_t rm1 = hb * la;
	uint64_t rl = la * lb;
	uint64_t t = rl + (rm0 << 32);
	uint64_t lo = t + (rm1 << 32);
	uint64_t c = (lo < t) + (t < rl);
	uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
	*A = lo;  *B = hi;
#endif
}

static inline uint64_t
_wymix(uint64_t A, uint64_t B)
{
	cf_mult128(&A, &B);
	return A ^ B;
}

static inline uint64_t
_wyr8(const uint8_t* p)
{
	return cf_swap_from_le64(*(uint64_t*)p);
}

static inline uint64_t
_wyr4(const uint8_t* p)
{
	return (uint64_t)cf_swap_from_le32(*(uint32_t*)p);
}

static inline uint64_t
_wyr3(const uint8_t* p, size_t k)
{
	// Note - this is endian independent.
	return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

// Public API - based on wyhash_final_version_3.
static inline uint64_t
cf_wyhash64(const void* key, size_t len)
{
	static const uint64_t secret[4] = {
			0xa0761d6478bd642f,
			0xe7037ed1a0b428db,
			0x8ebc6af09c88c6e3,
			0x589965cc75374cc3
	};

	// This seed is known to be good.
	uint64_t seed = 0x29FBB14cc886f ^ *secret;

	const uint8_t* p = (const uint8_t*)key;
	uint64_t a;
	uint64_t b;

	if (len <= 16) {
		if (len >= 4) {
			a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
			b = (_wyr4(p + len - 4) << 32) |
				_wyr4(p + len - 4 - ((len >> 3) << 2));
		}
		else if (len > 0) {
			a = _wyr3(p, len);
			b = 0;
		}
		else {
			a = b = 0;
		}
	}
	else {
		size_t i = len;

		if (i > 48) {
			uint64_t see1 = seed;
			uint64_t see2 = seed;

			do {
				seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
				see1 = _wymix(_wyr8(p + 16) ^ secret[2], _wyr8(p + 24) ^ see1);
				see2 = _wymix(_wyr8(p + 32) ^ secret[3], _wyr8(p + 40) ^ see2);
				p += 48;
				i -= 48;
			} while (i > 48);

			seed ^= see1 ^ see2;
		}

		while (i > 16) {
			seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
			i -= 16;
			p += 16;
		}

		a = _wyr8(p + i - 16);
		b = _wyr8(p + i - 8);
	}

	return _wymix(secret[1] ^ len, _wymix(a ^ secret[1], b ^ seed));
}

// Public API - for now just use low bits of 64-bit hash.
static inline uint32_t
cf_wyhash32(const void* key, size_t len)
{
	return (uint32_t)cf_wyhash64(key, len);
}

#ifdef __cplusplus
} // end extern "C"
#endif
