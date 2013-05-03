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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void * cf_malloc_at(size_t sz, char *file, int line);
void * cf_calloc_at(size_t nmemb, size_t sz, char *file, int line);
void * cf_realloc_at(void *ptr, size_t sz, char *file, int line);
void * cf_strdup_at(const char *s, char *file, int line);
void * cf_strndup_at(const char *s, size_t n, char *file, int line);
void * cf_valloc_at(size_t sz, char *file, int line);
void cf_free_at(void *p, char *file, int line);

void * cf_rc_alloc_at(size_t sz, char *file, int line);
void cf_rc_free_at(void *addr, char *file, int line);

cf_atomic_int_t cf_rc_count(void *addr);
int cf_rc_reserve(void *addr);
int cf_rc_release(void *addr);
int cf_rc_releaseandfree(void *addr);

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cf_malloc(s)            cf_malloc_at(s, __FILE__, __LINE__)
#define cf_calloc(nmemb, sz)    cf_calloc_at(nmemb, sz, __FILE__, __LINE__)
#define cf_realloc(ptr, sz)     cf_realloc_at(ptr, sz, __FILE__, __LINE__)
#define cf_strdup(s)            cf_strdup_at(s, __FILE__, __LINE__)
#define cf_strndup(s, n)        cf_strndup_at(s, n, __FILE__, __LINE__)
#define cf_valloc(sz)           cf_valloc_at(sz, __FILE__, __LINE__)
#define cf_free(p)              cf_free_at(p, __FILE__, __LINE__)

#define cf_rc_alloc(__a)        cf_rc_alloc_at((__a), __FILE__, __LINE__ )
#define cf_rc_free(__a)         cf_rc_free_at((__a), __FILE__, __LINE__ )

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif