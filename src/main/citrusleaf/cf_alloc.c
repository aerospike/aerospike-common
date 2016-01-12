/* 
 * Copyright 2008-2016 Aerospike, Inc.
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
 
/*
 *  NB:  Compile this default memory allocator only if the enhanced memory allocator is *NOT* enabled.
 */
#ifndef ENHANCED_ALLOC

#include <stdbool.h>
#include <string.h>

#include <citrusleaf/alloc.h>

void *cf_malloc(size_t sz) {
	return malloc(sz);
}

void *cf_calloc(size_t nmemb, size_t sz) {
	return calloc(nmemb, sz);
}

void *cf_realloc(void *ptr, size_t sz) {
	return realloc(ptr,sz);
}

void *cf_strdup(const char *s) {
	return strdup(s);
}

void *cf_strndup(const char *s, size_t n) {
	return strndup(s, n);
}

void *cf_valloc(size_t sz) {
	return valloc(sz);
}

void cf_free(void *p) {
	free(p);
}

/* cf_rc_count
 * Get the reservation count for a memory region */
cf_atomic_int_t cf_rc_count(void *addr) {
	cf_rc_counter *rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));

	return *rc;
}

/* cf_rc_reserve
 * Get a reservation on a memory region */
int cf_rc_reserve(void *addr) {
	cf_rc_counter *rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));
	int i = (int) cf_atomic32_add(rc, 1);

	smb_mb();

	return i;
}

/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with uint8_ts of value zero */
void *cf_rc_alloc(size_t sz)
{
	size_t asz = sizeof(cf_rc_counter) + sz;
	uint8_t *addr = malloc(asz);

	if (NULL == addr) {
		return NULL;
	}
	cf_atomic_int_set((cf_atomic_int *) addr, 1);

	return addr + sizeof(cf_rc_counter);
}

/* cf_rc_free
 * Deallocate a reference-counted memory region */
void cf_rc_free(void *addr) {
	cf_rc_counter *rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));
	free((void *) rc);
}

/* cf_rc_release
 * Release a reservation on a memory region */
static inline cf_atomic_int_t cf_rc_release_x(void *addr, bool autofree) {
	uint64_t c = 0;
	cf_rc_counter *rc = (cf_rc_counter *) (((uint8_t *)addr) - sizeof(cf_rc_counter));

	// Release the reservation; if this reduced the reference count to zero,
	// then free the block if autofree is set, and return 1.  Otherwise,
	// return 0
	smb_mb();
	if (0 == (c = cf_atomic32_decr(rc)) && autofree) {
		free((void *)rc);
	}

	return c;
}

int cf_rc_release(void *addr) {
	return (int)cf_rc_release_x(addr, false);
}

int cf_rc_releaseandfree(void *addr) {
	return (int)cf_rc_release_x(addr, true);
}

#endif // defined(ENHANCED_ALLOC)
