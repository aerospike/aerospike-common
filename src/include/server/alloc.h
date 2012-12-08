/*
 *  Citrusleaf Foundation
 *  include/alloc.h - memory allocation framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include "atomic.h"
#include "../cf_alloc.h"

#ifdef MEM_COUNT
#include "memcount.h"
#endif

typedef struct {
	cf_atomic32 count;
	uint32_t	sz;
} cf_rc_hdr;


extern void cf_rc_init();
extern int cf_rc_count(void *addr);
