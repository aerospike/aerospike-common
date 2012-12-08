/*
 *  Citrusleaf Foundation
 *  include/alloc.h - memory allocation framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include "client/cf_alloc.h"
#include "client/cf_atomic.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

typedef cf_atomic32 cf_rc_counter;


void * cf_malloc_at(size_t sz, char *file, int line) {
	return malloc(sz);
}

void * cf_calloc_at(size_t nmemb, size_t sz, char *file, int line) {
	return calloc(nmemb, sz);
}

void * cf_realloc_at(void *ptr, size_t sz, char *file, int line) {
	return realloc(ptr,sz);
}

void * cf_strdup_at(const char *s, char *file, int line) {
	return strdup(s);
}

void * cf_strndup_at(const char *s, size_t n, char *file, int line) {
	return strndup(s, n);
}

void * cf_valloc_at(size_t sz, char *file, int line) {
	return valloc(sz);
}

void cf_free_at(void *p, char *file, int line) {
	free(p);
}



/* cf_rc_count
 * Get the reservation count for a memory region */
cf_atomic_int_t cf_rc_count(void *addr) {
	cf_rc_counter * rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));
	return *rc;
}


/* cf_rc_reserve
 * Get a reservation on a memory region */
int cf_rc_reserve(void *addr) {
	cf_rc_counter * rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));
	int i = (int) cf_atomic32_add(rc, 1);
	smb_mb();
	return i;
}


/* cf_rc_release
 * Release a reservation on a memory region */
static inline cf_atomic_int_t cf_rc_release_x(void *addr, bool autofree) {
	uint64_t c = 0;
	cf_rc_counter * rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));
	// Release the reservation; if this reduced the reference count to zero,
	// then free the block if autofree is set, and return 1.  Otherwise,
	// return 0 
	smb_mb();
	if ( 0 == (c = cf_atomic32_decr(rc)) && autofree ){
		free((void *)rc);
	}
	return c;
}

int cf_rc_release(void *addr) {
	return cf_rc_release_x(addr,false);
}

int cf_rc_releaseandfree(void *addr) {
	return cf_rc_release_x(addr,true);
}

/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with uint8_ts of value zero */
void * cf_rc_alloc_at(size_t sz, char *file, int line)
{
	size_t asz = sizeof(cf_rc_counter) + sz;
	uint8_t * addr = malloc(asz);
	if (NULL == addr) return NULL;
	cf_atomic_int_set((cf_atomic_int *)addr, 1);
	return addr + sizeof(cf_rc_counter);
}


/* cf_rc_free
 * Deallocate a reference-counted memory region */
void cf_rc_free_at(void *addr, char *file, int line) {
	cf_rc_counter * rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));
	free((void *)rc);
	return;
}
