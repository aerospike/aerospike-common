/* 
 * Copyright 2008-2018 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#pragma once

#include <aerospike/as_std.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENHANCED_ALLOC

#include "enhanced_alloc.h"

#else // !defined(ENHANCED_ALLOC)

/*
 *  CF Memory Allocation-Related Functions:
 *
 *  These functions simply wrap the C standard library memory allocation-related functions.
 */

AS_EXTERN void* cf_malloc(size_t sz);
AS_EXTERN void* cf_calloc(size_t nmemb, size_t sz);
AS_EXTERN void* cf_realloc(void* ptr, size_t sz);
AS_EXTERN void* cf_strdup(const char* s);
AS_EXTERN void* cf_strndup(const char* s, size_t n);
AS_EXTERN void* cf_valloc(size_t sz);
AS_EXTERN void cf_free(void* p);

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
	uint32_t count;
	uint32_t sz;
} cf_rc_hdr;

void* cf_rc_alloc(size_t sz);
void cf_rc_free(void* addr);
uint32_t cf_rc_reserve(void* addr);
uint32_t cf_rc_release(void* addr);
uint32_t cf_rc_releaseandfree(void* addr);

#endif // defined(ENHANCED_ALLOC)

#ifdef __cplusplus
} // end extern "C"
#endif
