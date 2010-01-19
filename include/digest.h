/*
 *  Citrusleaf Foundation
 *  include/digest.h - message digests
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <openssl/ripemd.h>


/* SYNOPSIS
 * Cryptographic message digests
 * The intent is to provide an algorithm-neutral API for the computation of
 * cryptographic digests of arbitrary bytes.  Consequently, we define the
 * cf_digest type to be an array of bytes of the appropriate length.
 * The actual computation is done in one shot by calling cf_digest_compute().
 */



/* cf_digest
 * Storage for a message digest */
#define CF_DIGEST_KEY_SZ RIPEMD160_DIGEST_LENGTH
typedef struct { uint8_t digest[CF_DIGEST_KEY_SZ]; } cf_digest;

/* cf_digest_compute
 * Compute the digest of an input */
static inline void
cf_digest_compute(void *data, size_t len, cf_digest *d)
{
	RIPEMD160(data, len, (unsigned char *) d->digest);
}


// Compute a digest of two parts
// (often the set and the key)

static inline void
cf_digest_compute2(void *data1, size_t len1, void *data2, size_t len2, cf_digest *d)
{
	RIPEMD160_CTX c;
	RIPEMD160_Init(&c);
	RIPEMD160_Update(&c, data1, len1);
	RIPEMD160_Update(&c, data2, len2);
	RIPEMD160_Final( (unsigned char *)(d->digest), &c);
	
}

static inline uint32_t cf_digest_gethash(cf_digest *d, uint32_t MASK) 
{
	return((*(uint32_t *)d->digest) & MASK);
}

static inline uint32_t cf_digest_gethash_mod(cf_digest *d, uint32_t MOD) 
{
	return((*(uint32_t *)d->digest) % MOD);
}

