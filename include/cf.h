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


/* cf_compare_ptr
 * Compare the first sz bytes from two regions referenced by pointers */
static inline int
cf_compare_ptr(const void *a, const void *b, ssize_t sz)
{
	return(memcmp(a, b, sz));
}

// Sorry, too lazy to create a whole new file for just one function

extern int cf_id_get( uint64_t *id );

