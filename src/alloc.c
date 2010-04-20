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


#define EXTRA_CHECKS 1

/* cf_rc_count
 * Get the reservation count for a memory region */
int
cf_rc_count(void *addr)
{
	cf_rc_counter *rc;
	
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rccount: null address");
		return(0);
	}
#endif	

	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

	return((int) *rc);
}

/* notes regarding 'add' and 'decr' vs 'addunless' ---
** 
** A suggestion is to use 'addunless' in both the reserve and release code
** which you will see below. That use pattern causes somewhat better
** behavior in buggy use patterns, and allows asserts in cases where the
** calling code is behaving incorrectly. For example, if attempting to reserve a
** reference count where the count has already dropped to 0 (thus is free), 
** can be signaled with a message - and halted at 0, which is far safer.
** However, please not that using this functionality
** changes the API, as the return from 'addunless' is true or false (1 or 0)
** not the old value. Thus, the return from cf_rc_reserve and release will always
** be 0 or 1, instead of the current reference count number.
**
** As some of the citrusleaf client code currently uses the reference count
** as reserved, this code is using the 'add' and 'subtract'
*/


/* cf_rc_reserve
 * Get a reservation on a memory region */
int
cf_rc_reserve(void *addr)
{
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rcreserve: null address");
		return(0);
	}
#endif	
	cf_rc_counter *rc;	

	/* Extract the address of the reference counter, then increment it */
	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

#ifdef EXTRA_CHECKS
	// while not very atomic, does provide an interesting test
	// warning. While it might seem logical to check for 0 as well, the as fabric
	// system uses a perversion which doesn't use the standard semantics and will get
	// tripped up by this check
	if (*rc & 0x80000000) {
		cf_warning(CF_RCALLOC, "rcreserve: reserving without reference count, count %d very bad",*rc);
		return(0);
	}
#endif	
	
/* please see the previous note about add vs addunless
*/
	smb_mb();
	int i = (int) cf_atomic32_add(rc, 1);
	smb_mb();
	return(i);
}


/* _cf_rc_release
 * Release a reservation on a memory region */
int
_cf_rc_release(void *addr, bool autofree)
{
	cf_rc_counter *rc;
	int c;
	
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rcrelease: null address");
		return(-1); // don't tell misbehaving code to free null
	} 
#endif	

	/* Release the reservation; if this reduced the reference count to zero,
	 * then free the block if autofree is set, and return 1.  Otherwise,
	 * return 0
	 * Note that a straight decrement is less safe.  Consider the case where
	 * three threads are trying to operate on a block with reference count 1
	 * (yes this is ILLEGAL USE).  The first two threads try to release: the
	 * first one succeeds and frees the block (refcount 0), the second one
	 * succeeds and decrements the block (refcount -1) but doesn't free it
	 * (because the refcount is nonzero), and the third thread tries to
	 * acquire a reference and succeeds, making the refcount 0 on a block
	 * that has been freed but is also supposedly in use.  What a mess that
	 * would be. Using addunless allows the atomic checking that the reference
	 * count wasn't zero, so would result in multiple frees of the same object,
	 * which is an easier error to find and debug. 
	 * However, please see the earlier note about the API - whether this
	 * function should return 1 or 0 for having a reference count, or can/should
	 * return the actual count of the object */
/*	 
 	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));
    c = cf_atomic32_addunless(rc, 0, -1);
    if (0 == c)
        cf_warning(CF_RCALLOC, "rcrelease: double release attempted!");
    else if (autofree && (0 == cf_atomic32_get(*rc)))
        free((void*)rc);
*/        
	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));
	
	smb_mb();
	c = cf_atomic32_decr(rc);
	smb_mb();
#ifdef EXTRA_CHECKS
	if (c & 0xF0000000) {
		cf_warning(CF_RCALLOC, "rcrelease: releasing to a negative reference count");
		return(-1);
	}
#endif
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

	*(cf_atomic32 *)addr = 1; // doesn't have to be atomic
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
