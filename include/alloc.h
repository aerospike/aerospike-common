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


/* SYNOPSIS
 * Reference counting allocation
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

extern void cf_rc_init();

/* cf_rc_counter
 * A reference counter */
// typedef cf_atomic32 cf_rc_counter;

/* Function declarations */
extern int cf_rc_count(void *addr);

extern void *_cf_rc_alloc(size_t sz, char *file, int line);
#define cf_rc_alloc(__sz) (_cf_rc_alloc(__sz, __FILE__, __LINE__ ))

extern int _cf_rc_reserve(void *addr, char *file, int line);
#define cf_rc_reserve(__addr) (_cf_rc_reserve(__addr, __FILE__, __LINE__))

extern int _cf_rc_release(void *addr, bool autofree, char *file, int line);
#define cf_rc_release(__a) (_cf_rc_release((__a), FALSE, __FILE__, __LINE__ ))
#define cf_rc_releaseandfree(__a) (_cf_rc_release((__a), TRUE, __FILE__, __LINE__ ))

extern void _cf_rc_free(void *addr, char *file, int line);
#define cf_rc_free(__a) (_cf_rc_free((__a), __FILE__, __LINE__ ))

//
// A SIMPLE WAY OF ALLOCATING MEMORY
//
#if 1

#include <stdlib.h>

static inline void *cf_malloc(size_t s) {
	return(malloc(s));
}
static inline void cf_free(void *p) {
	free(p);
}
static inline void *cf_calloc(size_t nmemb, size_t sz) {
	return(calloc(nmemb, sz));
}
static inline void *cf_realloc(void *ptr, size_t sz) {
	return(realloc(ptr, sz));
}
static inline void *cf_strdup(const char *s) {
	return(strdup(s));
}
static inline void *cf_strndup(const char *s, size_t n) {
	return(strndup(s, n));
}
static inline void *cf_valloc(size_t sz) {
	void *r = 0;
	if (0 == posix_memalign( &r, 4096, sz)) return(r);
	return(0);
}
#endif

//
// A COMPLCIATED WAY
//

#if 0

extern void *   (*g_malloc_fn) (size_t s);
extern int      (*g_posix_memalign_fn) (void **memptr, size_t alignment, size_t sz);
extern void     (*g_free_fn) (void *p);
extern void *   (*g_calloc_fn) (size_t nmemb, size_t sz);
extern void *   (*g_realloc_fn) (void *p, size_t sz);
extern char *   (*g_strdup_fn) (const char *s);
extern char *   (*g_strndup_fn) (const char *s, size_t n);

static inline void *cf_malloc(size_t s) {
	return(g_malloc_fn(s));
}
static inline void *cf_valloc(size_t sz) {
	void *r = 0;
	if (0 == g_posix_memalign_fn( &r, 4096, sz)) return(r);
	return(0);
}
static inline void cf_free(void *p) {
	g_free_fn(p);
}
static inline void *cf_calloc(size_t nmemb, size_t sz) {
	return(g_calloc_fn(nmemb, sz));
}
static inline void *cf_realloc(void *ptr, size_t sz) {
	return(g_realloc_fn(ptr, sz));
}
static inline char *cf_strdup(const char *s) {
	return(g_strdup_fn(s));
}
static inline char *cf_strndup(const char *s, size_t n) {
	return(g_strndup_fn(s, n));
}

#endif


// extern void *cf_malloc(size_t sz);
// extern void cf_free(void *p);
// extern void cf_calloc(size_t nmemb, size_t sz);
// extern void *cf_realloc(void *ptr, size_t sz);
// extern char *strdup(const char *s)
// extern char *strndup(const char *s, size_t n);

// this is a good test to see we're not using forbidden functions
// except that one in signal.c
//
// IN ORDER TO USE THIS, you'll have to comment out (temporarily)
// the use of these functions in alloc.
//
#if 0

#define malloc(_x) DONTUSEMALLOC(_x)
#define posix_memalign(__x, __y, __z) DONTUSEMEMALIGN(__x, __y, __z)
#define memalign(__x, __y) DONTUSEMEMALIGN(__x, __y)
#define valloc(__x) DONTUSEVALLOC(__x)
#define free(_x)   DONTUSEFREE(_x)
#define calloc(_x, _y) DONTUSECALLOC(_x, _y)
#define realloc(_x, _y) DONTUSEREALLOC(_x, _y)
#undef strdup
#define strdup DONTUSESTRDUP
#undef strndup
#define strndup DONTUSESTRNDUP

#endif
