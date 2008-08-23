/*
 *  Citrusleaf Foundation
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include "atomic.h"
#include "alloc.h"
#include "digest.h"
#include "fault.h"
#include "lock.h"
#include "queue.h"
#include "rb.h"
#include "socket.h"

extern void  cf_rc_test();

/* Truth and falsehood */
#define FALSE 0
#define TRUE 1

/* cf_compare_ptr
 * Compare the first sz bytes from two regions referenced by pointers */
static inline int
cf_compare_ptr(const void *a, const void *b, ssize_t sz)
{
	return(memcmp(a, b, sz));
}
