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

/*
 * Cryptographic message digests
 * The intent is to provide an algorithm-neutral API for the computation of
 * cryptographic digests of arbitrary bytes.  Consequently, we define the
 * cf_digest type to be an array of bytes of the appropriate length.
 * The actual computation is done in one shot by calling cf_digest_compute().
 */

#include <stdint.h>
#include <stddef.h>
#include <openssl/ripemd.h>
#include <openssl/md4.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
// Openssl is deprecated on mac, but the library is still included.
// Since RIPEMD160 is not supported by the new mac common cryto system library,
// openssl is still used.  Save old settings and disable deprecated warnings.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

// #define DEBUG_VERBOSE 1

#define CF_DIGEST_KEY_SZ RIPEMD160_DIGEST_LENGTH

#define CF_SIGNATURE_SZ (sizeof(uint64_t))

/******************************************************************************
 * TYPES
 *****************************************************************************/

typedef struct cf_digest_s cf_digest;

typedef uint64_t cf_signature;

typedef uint16_t cl_partition_id;

/**
 * cf_digest_s
 * Storage for a message digest 
 */
struct cf_digest_s { 
	uint8_t 	digest[CF_DIGEST_KEY_SZ];
};

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

void cf_digest_string(cf_digest *digest, char* output);

/******************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

/**
 * cf_digest_compute
 * Compute the digest of an input 
 */
static inline void cf_digest_compute(const void *data, size_t len, cf_digest *d) {
	RIPEMD160((const unsigned char *) data, len, (unsigned char *) d->digest);
}


/**
 * Compute a digest of two parts
 * (often the set and the key)
 */
static inline void cf_digest_compute2(const void *data1, size_t len1, const void *data2, size_t len2, cf_digest *d) {
	if (len1 == 0) {
		RIPEMD160((const unsigned char *) data2, len2, (unsigned char *) d->digest);
	}
	else {
		RIPEMD160_CTX c;
		RIPEMD160_Init(&c);
		RIPEMD160_Update(&c, data1, len1);
		RIPEMD160_Update(&c, data2, len2);
		RIPEMD160_Final( (unsigned char *)(d->digest), &c);
	}
}

static inline uint32_t cf_digest_gethash(const cf_digest *d, uint32_t MASK)  {
	return((*(uint32_t *)d->digest) & MASK);
}

static inline uint32_t cf_digest_gethash_mod(const cf_digest *d, uint32_t MOD)  {
	return((*(uint32_t *)d->digest) % MOD);
}

/**
 * SIGNATURE
 * A non-crypto-solid signature
 */ 
static inline void cf_signature_compute(const void *data, size_t len, cf_signature *s) {
	uint8_t sig[MD4_DIGEST_LENGTH];

	MD4_CTX c;
	MD4_Init( &c );
	MD4_Update(&c, data, len);
	MD4_Final( (unsigned char *) &sig[0], &c);
	memcpy(s, sig, sizeof(*s));
}

/**
 * as_partition_getid
 * A brief utility function to derive the partition ID from a digest 
 */
static inline cl_partition_id cl_partition_getid(uint32_t n_partitions, const cf_digest *d) {
	uint16_t *d_int = (uint16_t *)&d->digest[0];
	cl_partition_id r = *d_int & (n_partitions - 1);
    return(r);
}

/*****************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif

#ifdef __APPLE__
// Restore old settings.
#pragma GCC diagnostic pop
#endif
