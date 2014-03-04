/*
 *  Citrusleaf Common
 *  include/alloc.h - memory allocation framework (default version)
 *
 *  Copyright 2008 - 2014 by Aerospike.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>

#define cf_malloc(s)            cf_malloc_at(s, __FILE__, __LINE__)
#define cf_calloc(nmemb, sz)    cf_calloc_at(nmemb, sz, __FILE__, __LINE__)
#define cf_realloc(ptr, sz)     cf_realloc_at(ptr, sz, __FILE__, __LINE__)
#define cf_strdup(s)            cf_strdup_at(s, __FILE__, __LINE__)
#define cf_strndup(s, n)        cf_strndup_at(s, n, __FILE__, __LINE__)
#define cf_valloc(sz)           cf_valloc_at(sz, __FILE__, __LINE__)
#define cf_free(p)              cf_free_at(p, __FILE__, __LINE__)

void *cf_malloc_at(size_t sz, char *file, int line);
void *cf_calloc_at(size_t nmemb, size_t sz, char *file, int line);
void *cf_realloc_at(void *ptr, size_t sz, char *file, int line);
void *cf_strdup_at(const char *s, char *file, int line);
void *cf_strndup_at(const char *s, size_t n, char *file, int line);
void *cf_valloc_at(size_t sz, char *file, int line);
void cf_free_at(void *p, char *file, int line);

/*
 * The "cf_rc_*()" Functions:  Reference Counting Allocation:
 *
 * This extends the traditional C memory allocation system to support
 * reference-counted garbage collection.  When a memory region is allocated
 * via cf_rc_alloc(), slightly more memory than was requested is actually
 * allocated.  A reference counter is inserted in the excess space at the
 * at the front of the region, and a pointer to the first byte of the data
 * allocation is returned.
 *
 * Two additional functions are supplied to support using a reference
 * counted region: cf_rc_reserve() reserves a memory region, and
 * cf_rc_release() releases an already-held reservation.  It is possible to
 * call cf_rc_release() on a region without first acquiring a reservation.
 * This will result in undefined behavior.
 */

void cf_rc_init(void);

#define cf_rc_alloc(__a)        cf_rc_alloc_at((__a), __FILE__, __LINE__)
#define cf_rc_free(__a)         cf_rc_free_at((__a), __FILE__, __LINE__)

void *cf_rc_alloc_at(size_t sz, char *file, int line);
void cf_rc_free_at(void *addr, char *file, int line);

volatile uint32_t cf_rc_count(void *addr);
int cf_rc_reserve(void *addr);
int cf_rc_release(void *addr);
int cf_rc_releaseandfree(void *addr);
