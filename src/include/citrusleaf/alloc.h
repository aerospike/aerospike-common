/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <stdlib.h>
#include <citrusleaf/cf_atomic.h>

#ifdef MEM_COUNT

#include "mem_count.h"

#endif // defined(MEM_COUNT)

/*
 *  Trivial hash function for storing 64-bit values in hash tables.
 */
static inline uint32_t ptr_hash_fn(void *key) {
    return (uint32_t) * (uint64_t *) key;
}

#ifdef ENHANCED_ALLOC

#include "enhanced_alloc.h"

#else // !defined(ENHANCED_ALLOC)

/*
 *  CF Memory Allocation-Related Functions:
 *
 *  These functions simply wrap the C standard library memory allocation-related functions.
 */

void *cf_malloc(size_t sz);
void *cf_calloc(size_t nmemb, size_t sz);
void *cf_realloc(void *ptr, size_t sz);
void *cf_strdup(const char *s);
void *cf_strndup(const char *s, size_t n);
void *cf_valloc(size_t sz);
void cf_free(void *p);

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

typedef cf_atomic32 cf_rc_counter;

typedef struct {
	cf_rc_counter count;
	uint32_t	sz;
} cf_rc_hdr;

void *cf_rc_alloc(size_t sz);
void cf_rc_free(void *addr);
cf_atomic_int_t cf_rc_count(void *addr);
int cf_rc_reserve(void *addr);
int cf_rc_release(void *addr);
int cf_rc_releaseandfree(void *addr);

#endif // defined(ENHANCED_ALLOC)
