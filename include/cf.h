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

#define CITRUSLEAF_EPOCH 1262304000

/* This one just looks better to me */
typedef uint8_t byte;
typedef unsigned int uint;


/* cf_bytearray
 * An array of bytes */
 /* TO AVOID CONFUSION, always create a cf_bytearray as a reference counted
    object! Always use them as reference counted objects! Always!!! */
struct cf_bytearray_t {
	uint64_t sz;
	byte data[];
};
typedef struct cf_bytearray_t cf_bytearray;




#define INVALID_FD (-1)

#include "atomic.h"
#include "dynbuf.h"
#include "fault.h"
#include "arena.h"
#include "arenah.h"
#include "alloc.h"
#include "bits.h"
#include "digest.h"
#include "lock.h"
#include "queue.h"
#include "rb.h"
#include "rcrb.h"
#include "socket.h"
#include "msg.h"
#include "rchash.h"
#include "shash.h"
#include "timer.h"
#include "ll.h"
#include "vector.h"
#include "cf_str.h"
#include "hist.h"
#include "hist_track.h"
#include "olock.h"
#include "clock.h"
#include "cf_random.h"
#include "meminfo.h"
#include "b64.h"
#include "rbuffer.h"
#include "vmap.h"
#include "cf_index.h"
#include "slist.h"
#include "ttree.h"



/* cf_hash_fnv
 * The 64-bit Fowler-Noll-Vo hash function (FNV-1a) */
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
 * Swap a 64-bit value
 * --- there's a fair amount of commentary on the web as to whether this kind
 * of swap optimization is useful for superscalar architectures.  Leave it in
 * for now */
#define cf_swap64(a, b) (void)(((a) == (b)) || (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))))
// #define cf_swap64(a, b) { uint64_t __t; __t = a; a = b; b = __t; }

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

/* cf_compare_uint64ptr
 * Compare two integers */
static inline int
cf_compare_uint64ptr(const void *pa, const void *pb)
{
    int r;
    const uint64_t *a = pa, *b = pb;

    if (*a == *b)
        return(0);

    r = (*a > *b) ? -1 : 1;

    return(r);
}


/* cf_compare_byteptr
 * Compare the regions pointed to by two sized byte pointers */
static inline int
cf_compare_byteptr(const void *pa, const size_t asz, const void *pb, const size_t bsz)
{
    if (asz != bsz)
        return(asz - bsz);

    return(memcmp(pa, pb, asz));
}

extern cf_digest cf_digest_zero;

// This one compare is probably the hottest line in the system.
// Thus, please benchmark with these different compare functions and
// see which is faster/better
#if 0
static inline int 
cf_digest_compare( cf_digest *d1, cf_digest *d2 )
{
	if (d1->digest[0] != d2->digest[0]) {
		if (d1->digest[0] < d2->digest[0])
			return(-1);
		else
			return(1);
	}
	return( memcmp( d1, d2, sizeof(cf_digest) ) );
}
#endif

#if 1
static inline int
cf_digest_compare( cf_digest *d1, cf_digest *d2 )
{
	return( memcmp( d1->digest, d2->digest, CF_DIGEST_KEY_SZ) );
}
#endif

// Sorry, too lazy to create a whole new file for just one function
#define CF_NODE_UNSET (0xFFFFFFFFFFFFFFFF)
typedef uint64_t cf_node;
extern uint32_t cf_nodeid_shash_fn(void *value);
extern uint32_t cf_nodeid_rchash_fn(void *value, uint32_t value_len);
typedef enum hb_mode_enum { AS_HB_MODE_UNDEF, AS_HB_MODE_MCAST, AS_HB_MODE_MESH } hb_mode_enum;
typedef enum hb_protocol_enum { AS_HB_PROTOCOL_UNDEF, AS_HB_PROTOCOL_NONE, AS_HB_PROTOCOL_V1, AS_HB_PROTOCOL_V2, AS_HB_PROTOCOL_RESET } hb_protocol_enum;
typedef enum paxos_protocol_enum { AS_PAXOS_PROTOCOL_UNDEF, AS_PAXOS_PROTOCOL_NONE, AS_PAXOS_PROTOCOL_V1, AS_PAXOS_PROTOCOL_V2 } paxos_protocol_enum;
typedef enum paxos_recovery_policy_enum { AS_PAXOS_RECOVERY_POLICY_UNDEF, AS_PAXOS_RECOVERY_POLICY_MANUAL, AS_PAXOS_RECOVERY_POLICY_AUTO_DUN_MASTER, AS_PAXOS_RECOVERY_POLICY_AUTO_DUN_ALL } paxos_recovery_policy_enum;
extern int cf_nodeid_get( unsigned short port, cf_node *id, char **node_ipp, hb_mode_enum hb_mode, char **hb_addrp, char **interface_names);
extern unsigned short cf_nodeid_get_port(cf_node id);

extern int cf_sort_firstk(uint64_t *v, size_t sz, int k);

extern void cf_process_daemonize(int *fd_ignore_list, int list_size);


/* Timekeeping */
typedef uint64_t cf_clock;
extern cf_clock cf_getms();
extern cf_clock cf_getus();
extern cf_clock cf_clock_getabsolute();
extern cf_clock cf_get_seconds();
extern cf_clock cf_secs_since_clepoch();

/* daemon.c */
extern void cf_process_privsep(uid_t uid, gid_t gid);

/* a mutex timeout macro */
static inline int 
cf_mutex_timedlock(pthread_mutex_t *lock, uint ms)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	TIMESPEC_ADD_MS(&ts,ms);
	return( pthread_mutex_timedlock( lock, &ts) );
}
