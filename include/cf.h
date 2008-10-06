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

/* Truth and falsehood !!! use C99 bool types instead, dude !!! */
#define FALSE 0
#define TRUE 1


/* Basic definitions */
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef uint8_t byte;

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

/* cf_bytearray
 * An array of bytes */
struct cf_bytearray_t {
	uint64_t sz;
	void *data;
};
typedef struct cf_bytearray_t cf_bytearray;


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

