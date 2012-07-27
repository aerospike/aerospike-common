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


//
// A SIMPLE WAY OF ALLOCATING MEMORY
//
#if 1

#include <stdlib.h>
#include <string.h>

#ifdef MEM_COUNT

/*
 * Type for selecting the field to be sorted on for memory count reporting.
 */
typedef enum sort_field_e {
	CF_ALLOC_SORT_NET_SZ,
	CF_ALLOC_SORT_DELTA_SZ,
	CF_ALLOC_SORT_NET_ALLOC_COUNT,
	CF_ALLOC_SORT_TOTAL_ALLOC_COUNT,
	CF_ALLOC_SORT_TIME_LAST_MODIFIED
} sort_field_t;

extern struct shash_s *mem_count_shash;
extern cf_atomic64 mem_count;
extern cf_atomic64 mem_count_mallocs;
extern cf_atomic64 mem_count_frees;
extern cf_atomic64 mem_count_callocs;
extern cf_atomic64 mem_count_reallocs;
extern cf_atomic64 mem_count_strdups;
extern cf_atomic64 mem_count_strndups;
extern cf_atomic64 mem_count_vallocs;

#define cf_malloc(s) (cf_malloc_count(s, __FILE__, __LINE__))
#define cf_free(p) (cf_free_count(p, __FILE__, __LINE__))
#define cf_calloc(nmemb, sz) (cf_calloc_count(nmemb, sz, __FILE__, __LINE__))
#define cf_realloc(ptr, sz) (cf_realloc_count(ptr, sz, __FILE__, __LINE__))
#define cf_strdup(s) (cf_strdup_count(s, __FILE__, __LINE__))
#define cf_strndup(s, n) (cf_strndup_count(s, n, __FILE__, __LINE__))
#define cf_valloc(sz) (cf_valloc_count(sz, __FILE__, __LINE__))

extern int mem_count_init();
extern void mem_count_stats();
extern int mem_count_alloc_info(char *file, int line, cf_dyn_buf *db);
extern void mem_count_report(sort_field_t sort_field, int top_n, cf_dyn_buf *db);
extern void mem_count_shutdown();
extern void *cf_malloc_count(size_t sz, char *file, int line);
extern void cf_free_count(void *p, char *file, int line);
extern void *cf_calloc_count(size_t nmemb, size_t sz, char *file, int line);
extern void *cf_realloc_count(void *ptr, size_t sz, char *file, int line);
extern void *cf_strdup_count(const char *s, char *file, int line);
extern void *cf_strndup_count(const char *s, size_t n, char *file, int line);
extern void *cf_valloc_count(size_t sz, char *file, int line);

static inline uint32_t ptr_hash_fn(void *key) {
	return((uint32_t)(*(uint64_t *)key));
}

#elif defined(MEM_TRACK)

typedef struct mem_track_alloc_s {
	int sz;
	char file[100];
	int line;
} mem_track_alloc;

extern struct shash_s *mem_alloced;
extern cf_atomic64 mem_alloced_sum;

static inline uint32_t ptr_hash_fn(void *key) {
	return((uint32_t)(*(uint64_t *)key));
}

#define cf_malloc(s) (cf_malloc_track(s, __FILE__, __LINE__))
#define cf_free(p) (cf_free_track(p, __FILE__, __LINE__))
#define cf_calloc(nmemb, sz) (cf_calloc_track(nmemb, sz, __FILE__, __LINE__))
#define cf_realloc(ptr, sz) (cf_realloc_track(ptr, sz, __FILE__, __LINE__))
#define cf_strdup(s) (cf_strdup_track(s, __FILE__, __LINE__))
#define cf_strndup(s, n) (cf_strndup_track(s, n, __FILE__, __LINE__))
#define cf_valloc(sz) (cf_valloc_track(sz, __FILE__, __LINE__))

extern void *cf_malloc_track(size_t sz, char *file, int line);
extern void cf_free_track(void *p, char *file, int line);
extern void *cf_calloc_track(size_t nmemb, size_t sz, char *file, int line);
extern void *cf_realloc_track(void *ptr, size_t sz, char *file, int line);
extern void *cf_strdup_track(const char *s, char *file, int line);
extern void *cf_strndup_track(const char *s, size_t n, char *file, int line);
extern void *cf_valloc_track(size_t sz, char *file, int line);

#else // !defined(MEM_TRACK)

static inline void *cf_malloc(size_t sz) {
	return(malloc(sz));
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
	void *p = 0;
	if (0 == posix_memalign( &p, 4096, sz)) {
		return(p);
	}
	return(0);
}

#endif // defined(MEM_TRACK)

#endif

//
// A COMPLICATED WAY
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

/* cf_rc_counter
 * A reference counter */
// typedef cf_atomic32 cf_rc_counter;

typedef struct {
	cf_atomic32 count;
	uint32_t	sz;
} cf_rc_hdr;


/* Function declarations */
extern int cf_rc_count(void *addr);

extern void *_cf_rc_alloc(size_t sz, char *file, int line);
#define cf_rc_alloc(__sz) (_cf_rc_alloc(__sz, __FILE__, __LINE__ ))

extern void _cf_rc_free(void *addr, char *file, int line);
#define cf_rc_free(__a) (_cf_rc_free((__a), __FILE__, __LINE__ ))

#ifdef MEM_TRACK

// this is for tracking
extern int _cf_rc_reserve(void *addr, char *file, int line);

// for tracking
extern int _cf_rc_release(void *addr, bool autofree, char *file, int line);

#define cf_rc_reserve(__addr) (_cf_rc_reserve(__addr, __FILE__, __LINE__))
#define cf_rc_release(__a) (_cf_rc_release((__a), FALSE, __FILE__, __LINE__ ))
#define cf_rc_releaseandfree(__a) (_cf_rc_release((__a), TRUE, __FILE__, __LINE__ ))

#else // !defined(MEM_TRACK)

extern int _cf_rc_reserve(void *addr);
extern int _cf_rc_release(void *addr);
extern int _cf_rc_releaseandfree(void *addr);

#define cf_rc_reserve(__addr) (_cf_rc_reserve(__addr))
#define cf_rc_release(__a) (_cf_rc_release(__a))
#define cf_rc_releaseandfree(__a) (_cf_rc_release(__a))

#endif // defined(MEM_TRACK)
