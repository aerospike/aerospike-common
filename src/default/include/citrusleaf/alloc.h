/*
 *  Citrusleaf Foundation
 *  include/alloc.h - memory allocation framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once

#include <stdlib.h>
#include <citrusleaf/cf_atomic.h>

#ifdef MEM_COUNT

#include <string.h>
#include <citrusleaf/dynbuf.h>

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

/*
 *  Type for mode of enabling / disabling memory accounting.
 */
typedef enum mem_count_mode_e {
	MEM_COUNT_DISABLE,            // Disable memory accounting.
	MEM_COUNT_ENABLE,             // Enable memory accounting at daemon start-up time.
	MEM_COUNT_ENABLE_DYNAMIC      // Enable memory accounting at run-time.
} mem_count_mode_t;

extern struct shash_s *mem_count_shash;
extern cf_atomic64 mem_count;
extern cf_atomic64 mem_count_mallocs;
extern cf_atomic64 mem_count_frees;
extern cf_atomic64 mem_count_callocs;
extern cf_atomic64 mem_count_reallocs;
extern cf_atomic64 mem_count_strdups;
extern cf_atomic64 mem_count_strndups;
extern cf_atomic64 mem_count_vallocs;

extern int mem_count_init(mem_count_mode_t mode);
extern void mem_count_stats();
extern int mem_count_alloc_info(char *file, int line, cf_dyn_buf *db);
extern int mem_count_report(sort_field_t sort_field, int top_n, cf_dyn_buf *db);
extern void mem_count_shutdown();

#endif // defined(MEM_COUNT)

/*
 *  Trivial hash function for storing 64-bit values in hash tables.
 */
static inline uint32_t ptr_hash_fn(void *key) {
    return (uint32_t) * (uint64_t *) key;
}

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

typedef struct {
	cf_atomic32 count;
	uint32_t	sz;
} cf_rc_hdr;

void cf_rc_init();

#define cf_rc_alloc(__a)        cf_rc_alloc_at((__a), __FILE__, __LINE__)
#define cf_rc_free(__a)         cf_rc_free_at((__a), __FILE__, __LINE__)

void *cf_rc_alloc_at(size_t sz, char *file, int line);
void cf_rc_free_at(void *addr, char *file, int line);

cf_atomic_int_t cf_rc_count(void *addr);
int cf_rc_reserve(void *addr);
int cf_rc_release(void *addr);
int cf_rc_releaseandfree(void *addr);

#ifdef PREPRO

// For generating code via the C pre-processor:

#define cf_calloc(nmemb, size) (GEN_TAG AddLoc("cf_calloc_count", __FILE__, __LINE__) GEN_TAG)
#define cf_malloc(size) (GEN_TAG AddLoc("cf_malloc_count", __FILE__, __LINE__) GEN_TAG)
#define cf_free(ptr) (GEN_TAG AddLoc("cf_free_count", __FILE__, __LINE__) GEN_TAG)
#define cf_realloc(ptr, size) (GEN_TAG AddLoc("cf_realloc_count", __FILE__, __LINE__) GEN_TAG)
#define cf_strdup(s) (GEN_TAG AddLoc("cf_strdup_count", __FILE__, __LINE__) GEN_TAG)
#define cf_strndup(s, n) (GEN_TAG AddLoc("cf_strndup_count", __FILE__, __LINE__) GEN_TAG)
#define cf_valloc(size) (GEN_TAG AddLoc("cf_valloc_count", __FILE__, __LINE__) GEN_TAG)

#elif defined(USE_ASM)

/*
 *  Type representing the state of a memory allocation location in a program.
 */
typedef struct as_mallocation_s {
	ssize_t total_size;                   // Cumulative net total size allocated by this thread.
	ssize_t delta_size;                   // Most recent change in size.
	ssize_t last_size;                    // Total size last reported change from this location.
	struct timespec last_time;            // Time of last reported change from this location.
	uint16_t type;                        // Type of the last memory allocation-related operation.
	uint16_t loc;                         // Location of the allocation in the program.
} __attribute__((__packed__)) as_mallocation_t;

/*
 *  Type representing a unique location in the program where a memory allocation-related function is called.
 */
typedef uint16_t malloc_loc_t;

void *cf_calloc_loc(size_t nmemb, size_t size, malloc_loc_t loc);
void *cf_malloc_loc(size_t size, malloc_loc_t loc);
void cf_free_loc(void *ptr, malloc_loc_t loc);
void *cf_realloc_loc(void *ptr, size_t size, malloc_loc_t loc);
char *cf_strdup_loc(const char *s, malloc_loc_t loc);
char *cf_strndup_loc(const char *s, size_t n, malloc_loc_t loc);
void *cf_valloc_loc(size_t size, malloc_loc_t loc);

// Note:  These are function pointer variables and therefore must be "extern"!
extern void (*g_mallocation_set)(uint16_t type, uint16_t loc, ssize_t delta_size);
extern void (*g_mallocation_get)(uint16_t *type, uint16_t loc, ssize_t *total_size, ssize_t *delta_size, struct timespec *last_time);

void my_cb(uint64_t thread_id, uint16_t type, uint16_t loc, ssize_t delta_size, ssize_t total_size, struct timespec *last_time, void *udata);

#else

// The original (shash-based) memory accounting system:

void *cf_malloc_at(size_t sz, char *file, int line);
void *cf_calloc_at(size_t nmemb, size_t sz, char *file, int line);
void *cf_realloc_at(void *ptr, size_t sz, char *file, int line);
void *cf_strdup_at(const char *s, char *file, int line);
void *cf_strndup_at(const char *s, size_t n, char *file, int line);
void *cf_valloc_at(size_t sz, char *file, int line);
void cf_free_at(void *p, char *file, int line);

#define cf_malloc(s)            cf_malloc_at(s, __FILE__, __LINE__)
#define cf_calloc(nmemb, sz)    cf_calloc_at(nmemb, sz, __FILE__, __LINE__)
#define cf_realloc(ptr, sz)     cf_realloc_at(ptr, sz, __FILE__, __LINE__)
#define cf_strdup(s)            cf_strdup_at(s, __FILE__, __LINE__)
#define cf_strndup(s, n)        cf_strndup_at(s, n, __FILE__, __LINE__)
#define cf_valloc(sz)           cf_valloc_at(sz, __FILE__, __LINE__)
#define cf_free(p)              cf_free_at(p, __FILE__, __LINE__)

#endif
