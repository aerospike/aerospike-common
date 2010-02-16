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


/* cf_rc_count
 * Get the reservation count for a memory region */
int
cf_rc_count(void *addr)
{
	cf_rc_counter *rc;
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rccount: null address");
		return(0);
	}

	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

	return((int) *rc);
}


/* cf_rc_reserve
 * Get a reservation on a memory region */
int
cf_rc_reserve(void *addr)
{
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rcreserve: null address");
		return(0);
	}
	cf_rc_counter *rc;	

	/* Extract the address of the reference counter, then increment it */
	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

    /* An add-unless-zero is required here to avoid a race: a release might
     * be in flight, which might result in a free() on a reserved region. */
	return((int)cf_atomic32_addunless(rc, 0, 1));
//	return((int) cf_atomic32_add(rc, 1));
}


/* _cf_rc_release
 * Release a reservation on a memory region */
int
_cf_rc_release(void *addr, bool autofree)
{
	cf_rc_counter *rc;
	int c;
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rcrelease: null address");
		return(0);
	} 

	/* Release the reservation; if this reduced the reference count to zero,
	 * then free the block if autofree is set, and return 1.  Otherwise,
	 * return 0 */
	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));
	c = cf_atomic32_decr(rc);
	if ((0 == c) && autofree)
			free((void *)rc);

	return(c);
}


/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with bytes of value zero */
void *
cf_rc_alloc(size_t sz)
{
	uint8_t *addr;
	size_t asz = sizeof(cf_rc_counter) + sz;

	addr = malloc(asz);
	if (NULL == addr)
		return(NULL);

	cf_atomic32_set((cf_atomic32 *)addr, 1);
	byte *base = addr + sizeof(cf_rc_counter);

	return(base);
}


/* cf_rc_free
 * Deallocate a reference-counted memory region */
void
cf_rc_free(void *addr)
{
	cf_rc_counter *rc;
	cf_assert(addr, CF_RCALLOC, CF_PROCESS, CF_CRITICAL, "null address");

	rc = (cf_rc_counter *) (  ((uint8_t *)addr) - sizeof(cf_rc_counter) );

	cf_assert(cf_atomic32_get(*(cf_atomic32 *)rc) == 0,
		CF_RCALLOC, CF_PROCESS, CF_CRITICAL, "attempt to free reserved object");

	free((void *)rc);

	return;
}
