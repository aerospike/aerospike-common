/*
 *  Citrusleaf Foundation
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once

// requird for memcmp
#include <string.h>
// required for true and false and bool
#include <stdbool.h>
// We have lots of use of basic types like uint8_t
#include <inttypes.h>
// Some people use ssize_t (signed size), which is in unistd.h
#include <unistd.h>

/* Truth and falsehood !!! use C99 bool types instead, dude !!! */
#define FALSE 0
#define TRUE 1


/* Basic definitions */
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef uint8_t byte;

/* cf_bytearray
 * An array of bytes */
struct cf_bytearray_t {
	uint64_t sz;
	byte data[];
};
typedef struct cf_bytearray_t cf_bytearray;


#include "atomic.h"
#include "alloc.h"
#include "digest.h"
#include "fault.h"
#include "lock.h"
#include "queue.h"
#include "rb.h"
#include "socket.h"
#include "msg.h"
#include "bbhash.h"
#include "rchash.h"

/* cf_hash_fnv
 * The 64-bit Fowler-Noll-Voll hash function (FNV-1a) */
static inline uint64_t
cf_hash_fnv(void *buf, size_t bufsz)
{
	uint64_t hash = 0xcbf29ce484222325ULL;
	uint8_t *bufp = (uint8_t *)buf, *bufe = bufp + bufsz;

	while (bufp < bufe) {
		/* XOR the current byte into the bottom of the hash */
		hash ^= (uint64_t)*bufp++;

		/* Multiply by the 64-bit FNV magic prime */
		hash *= 0x100000001b3ULL;
	}

	return(hash);
}


/* cf_hash_oneatatime
 * The 64-bit One-at-a-Time hash function */
static inline uint64_t
cf_hash_oneatatime(void *buf, size_t bufsz)
{
	size_t i;
	uint64_t hash = 0;
	uint8_t *b = (uint8_t *)buf;

	for (i = 0; i < bufsz; i++) {
		hash += b[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return(hash);
}


/* cf_swap64
 * Swap a 64-bit value */
#define cf_swap64(a, b) (((a) == (b)) || (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))))


/* cf_contains64
 * See if a vector of uint64s contains an certain value */
static inline int
cf_contains64(uint64_t *v, int vsz, uint64_t a)
{
    for (int i = 0; i < vsz; i++) {
        if (v[i] == a)
            return(1);
    }

    return(0);
}

/* cf_compare_ptr
 * Compare the first sz bytes from two regions referenced by pointers */
static inline int
cf_compare_ptr(const void *a, const void *b, ssize_t sz)
{
	return(memcmp(a, b, sz));
}

// Sorry, too lazy to create a whole new file for just one function
#define CF_NODE_UNSET (0xFFFFFFFFFFFFFFFF)
typedef uint64_t cf_node;
extern uint32 cf_nodeid_hash_fn(void *value, uint32 value_len);
extern int cf_nodeid_get( unsigned short port, cf_node *id );
extern unsigned short cf_nodeid_get_port(cf_node id);

extern void cf_sort_firstk(uint64_t *v, size_t sz, int k);

// This is even shorter!
extern uint64_t cf_getms();
