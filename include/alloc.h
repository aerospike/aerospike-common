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

//#define TRACK_MEM_ALLOC

#ifndef TRACK_MEM_ALLOC

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

#else

typedef struct mem_alloc_track_s {
	int sz;
	char file[100];
	int line;
} mem_alloc_track;

extern struct shash_s *mem_alloced;
extern cf_atomic64 mem_alloced_sum;

static inline uint32_t ptr_hash_fn(void *key) {
	return((uint32_t)(*(uint64_t *)key));
}

static inline int mem_alloced_reduce_fn(void *key, void *data, void *udata) {
	mem_alloc_track *p_mat = (mem_alloc_track *)data;
	cf_info(AS_INFO, "%p | %d (%s:%d)", *(void **)key, p_mat->sz, p_mat->file, p_mat->line);
	return(0);
}

#define cf_malloc(s) (cf_malloc_track(s, __FILE__, __LINE__))

static inline void *cf_malloc_track(size_t sz, char *file, int line) {
	void *p = malloc(sz);

	mem_alloc_track mat;
	mat.sz = sz;
	strcpy(mat.file, file);
	mat.line = line;
	shash_put(mem_alloced, &p, &mat);
	cf_atomic64_add(&mem_alloced_sum, sz);

	return(p);
}

#define cf_free(p) (cf_free_track(p, __FILE__, __LINE__))

static inline void cf_free_track(void *p, char *file, int line) {
	mem_alloc_track mat;
	shash_get_and_delete(mem_alloced, &p, &mat);
	cf_atomic64_sub(&mem_alloced_sum, mat.sz);

	free(p);
}

#define cf_calloc(nmemb, sz) (cf_calloc_track(nmemb, sz, __FILE__, __LINE__))

static inline void *cf_calloc_track(size_t nmemb, size_t sz, char *file, int line) {
	void *p = calloc(nmemb, sz);

	mem_alloc_track mat;
	mat.sz = sz;
	strcpy(mat.file, file);
	mat.line = line;
	shash_put(mem_alloced, &p, &mat);
	cf_atomic64_add(&mem_alloced_sum, sz);

	return(p);
}

#define cf_realloc(ptr, sz) (cf_realloc_track(ptr, sz, __FILE__, __LINE__))

static inline void *cf_realloc_track(void *ptr, size_t sz, char *file, int line) {
	void *p = realloc(ptr, sz);

	if (p != ptr) {
		mem_alloc_track mat;
		shash_get_and_delete(mem_alloced, &p, &mat);
		int64_t memory_delta = sz - mat.sz;

		mat.sz = sz;
		strcpy(mat.file, file);
		mat.line = line;
		shash_put(mem_alloced, &p, &mat);

		if (memory_delta) {
			cf_atomic64_add(&mem_alloced_sum, memory_delta);
		}
	} else {
		mem_alloc_track *p_mat;
		pthread_mutex_t *mem_alloced_lock;
		shash_get_vlock(mem_alloced, &p, (void **)&p_mat, &mem_alloced_lock);
		int64_t memory_delta = sz - p_mat->sz;

		p_mat->sz = sz;
		strcpy(p_mat->file, file);
		p_mat->line = line;

		pthread_mutex_unlock(mem_alloced_lock);

		if (memory_delta) {
			cf_atomic64_add(&mem_alloced_sum, memory_delta);
		}
	}

	return(p);
}

#define cf_strdup(s) (cf_strdup_track(s, __FILE__, __LINE__))

static inline void *cf_strdup_track(const char *s, char *file, int line) {
	void *p = strdup(s);

	mem_alloc_track mat;
	mat.sz = strlen(p) + 1;
	strcpy(mat.file, file);
	mat.line = line;
	shash_put(mem_alloced, &p, &mat);
	cf_atomic64_add(&mem_alloced_sum, mat.sz);

	return(p);
}

#define cf_strndup(s, n) (cf_strndup_track(s, n, __FILE__, __LINE__))

static inline void *cf_strndup_track(const char *s, size_t n, char *file, int line) {
	void *p = strndup(s, n);

	mem_alloc_track mat;
	mat.sz = strlen(p) + 1;
	strcpy(mat.file, file);
	mat.line = line;
	shash_put(mem_alloced, &p, &mat);
	cf_atomic64_add(&mem_alloced_sum, mat.sz);

	return(p);
}

#define cf_valloc(sz) (cf_valloc_track(sz, __FILE__, __LINE__))

static inline void *cf_valloc_track(size_t sz, char *file, int line) {
	void *p = 0;
	if (0 == posix_memalign( &p, 4096, sz)) {
		mem_alloc_track mat;
		mat.sz = sz;
		strcpy(mat.file, file);
		mat.line = line;
		shash_put(mem_alloced, &p, &mat);
		cf_atomic64_add(&mem_alloced_sum, sz);

		return(p);
	}
	return(0);
}
#endif

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

// this is for tracking
//extern int _cf_rc_reserve(void *addr, char *file, int line);
//#define cf_rc_reserve(__addr) (_cf_rc_reserve(__addr, __FILE__, __LINE__))
static inline 
int cf_rc_reserve(void *addr)
{
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	int i = (int) cf_atomic32_add(&hdr->count, 1);
	return(i);
}

// for tracking
// extern int _cf_rc_release(void *addr, bool autofree, char *file, int line);
// #define cf_rc_release(__a) (_cf_rc_release((__a), FALSE, __FILE__, __LINE__ ))
// #define cf_rc_releaseandfree(__a) (_cf_rc_release((__a), TRUE, __FILE__, __LINE__ ))

static inline
int cf_rc_release(void *addr) {
	int c;
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	c = cf_atomic32_decr(&hdr->count);
	return(c);
}

static inline
int cf_rc_releaseandfree(void *addr) {
	int c;
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	c = cf_atomic32_decr(&hdr->count);
	if (0 == c) {
		cf_free((void *)hdr);
	}
	return(c);
}

extern void _cf_rc_free(void *addr, char *file, int line);
#define cf_rc_free(__a) (_cf_rc_free((__a), __FILE__, __LINE__ ))

