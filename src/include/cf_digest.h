/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

/**
 * Cryptographic message digests
 * The intent is to provide an algorithm-neutral API for the computation of
 * cryptographic digests of arbitrary bytes.  Consequently, we define the
 * cf_digest type to be an array of bytes of the appropriate length.
 * The actual computation is done in one shot by calling cf_digest_compute().
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <openssl/ripemd.h>
#include <openssl/md4.h>
#include <stdio.h>
#include <string.h>

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

// #define DEBUG_VERBOSE 1

#define CF_DIGEST_KEY_SZ RIPEMD160_DIGEST_LENGTH

#define CF_SIGNATURE_SZ (sizeof(uint64_t))

/******************************************************************************
 * TYPES
 ******************************************************************************/

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
 * INLINE FUNCTIONS
 ******************************************************************************/

/**
 * cf_digest_compute
 * Compute the digest of an input 
 */
static inline void cf_digest_compute(void *data, size_t len, cf_digest *d) {
	RIPEMD160(data, len, (unsigned char *) d->digest);

#ifdef DEBUG_VERBOSE	
	fprintf(stderr, "digest computation: len %zu\n",len);
	uint8_t *buf = data;
	int i;
	for (i=0;i<len;i++) {
		fprintf(stderr, "%02x ",buf[i]);
		if (i % 16 == 15) fprintf(stderr, "\n");
	}
	if (i % 16 != 0) fprintf(stderr, "\n");
	
	fprintf(stderr, "digest output : %"PRIx64"\n",*(uint64_t *)d);
#endif	
}


/**
 * Compute a digest of two parts
 * (often the set and the key)
 */
static inline void cf_digest_compute2(void *data1, size_t len1, void *data2, size_t len2, cf_digest *d) {
	RIPEMD160_CTX c;
	RIPEMD160_Init(&c);
	RIPEMD160_Update(&c, data1, len1);
	RIPEMD160_Update(&c, data2, len2);
	RIPEMD160_Final( (unsigned char *)(d->digest), &c);
	
#ifdef DEBUG_VERBOSE	
	fprintf(stderr, "digest computation2: part1 len %zu\n",len1);
	uint8_t *buf = data1;
	int i;
	for (i=0;i<len1;i++) {
		fprintf(stderr, "%02x ",buf[i]);
		if (i % 16 == 15) fprintf(stderr, "\n");
	}
	if (i % 16 != 0) fprintf(stderr, "\n");
	
	fprintf(stderr, "digest computation2: part2 len %zu\n",len2);
	buf = data2;
	for (i=0;i<len2;i++) {
		fprintf(stderr, "%02x ",buf[i]);
		if (i % 16 == 15) fprintf(stderr, "\n");
	}
	if (i % 16 != 0) fprintf(stderr, "\n");
	
	fprintf(stderr, "digest output : %"PRIx64"\n",*(uint64_t *)d);
#endif	
}

static inline uint32_t cf_digest_gethash(cf_digest *d, uint32_t MASK)  {
	return((*(uint32_t *)d->digest) & MASK);
}

static inline uint32_t cf_digest_gethash_mod(cf_digest *d, uint32_t MOD)  {
	return((*(uint32_t *)d->digest) % MOD);
}

/**
 * SIGNATURE
 * A non-crypto-solid signature
 */ 
static inline void cf_signature_compute(void *data, size_t len, cf_signature *s) {
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
static inline cl_partition_id cl_partition_getid(uint32_t n_partitions, cf_digest *d) {
	uint16_t *d_int = (uint16_t *)&d->digest[0];
	cl_partition_id r = *d_int & (n_partitions - 1);
    return(r);
}
