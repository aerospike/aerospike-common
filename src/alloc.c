/*
 *  Citrusleaf Foundation
 *  include/alloc.h - memory allocation framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"

/* test */
void
cf_rc_test()
{
	printf("hello cruel library\n");
}

/* cf_rc_count
 * Get the reservation count for a memory region */
void
cf_rc_count(void *addr)
{
	cf_rc_counter *rc;

	rc = addr - sizeof(cf_rc_counter);

	return;
}


/* cf_rc_reserve_region
 * Get a reservation on a memory region */
int
cf_rc_reserve_region(void *addr, int initial)
{
	cf_rc_counter *rc;

	cf_assert(addr, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid argument");

	/* Extract the address of the reference counter, then increment it */
	rc = addr - sizeof(cf_rc_counter);
	if (initial)
		return(cf_atomic64_add(rc, 1));
	else
		return(cf_atomic64_addunless(rc, 0, 1));
}


/* cf_rc_release
 * Release a reservation on a memory region */
int
cf_rc_release(void *addr)
{
	cf_rc_counter *rc;

	cf_assert(addr, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid argument");

	/* Release the reservation; if this reduced the reference count to zero,
	 * then free the block */
	rc = addr - sizeof(cf_rc_counter);
	cf_atomic64_decr(rc);
	if (0 == (int64_t)cf_atomic64_get(*(cf_atomic64 *)rc))
		free((void *)rc);

	return(0);
}


/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with bytes of value zero */
void *
cf_rc_alloc(size_t sz)
{
	void *addr;
	size_t asz = sizeof(cf_rc_counter) + sz;

	addr = malloc(asz);
	if (NULL == addr)
		return(NULL);

	cf_atomic64_set((cf_atomic64 *)addr, 0);
	bzero(addr, sizeof(asz));

	return(addr + sizeof(cf_rc_counter));
}


/* cf_rc_free
 * Deallocate a reference-counted memory region */
void
cf_rc_free(void *addr)
{
	cf_rc_counter *rc;

	rc = addr - sizeof(cf_rc_counter);

	if (0 == (int64_t)cf_atomic64_get(*(cf_atomic64 *)rc))
		free(addr);

	return;
}
