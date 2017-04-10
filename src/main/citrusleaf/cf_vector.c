/*
 * Copyright 2008-2017 Aerospike, Inc.
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

//==========================================================
// Includes.
//

#include <string.h>
#include <stdlib.h>

#include <citrusleaf/cf_vector.h>

#include <citrusleaf/alloc.h>


//==========================================================
// Constants & typedefs.
//

#define VECTOR_FLAG_FREE_SELF   0x10
#define VECTOR_FLAG_FREE_VECTOR 0x20


//==========================================================
// Forward declarations.
//

static bool vector_resize(cf_vector *v, uint32_t new_count);


//==========================================================
// Macros.
//

#define VECTOR_LOCK(_v) if (v->flags & VECTOR_FLAG_BIGLOCK) { \
		pthread_mutex_lock(&((cf_vector *)_v)->LOCK); \
}

#define VECTOR_UNLOCK(_v) if (v->flags & VECTOR_FLAG_BIGLOCK) { \
		pthread_mutex_unlock(&((cf_vector *)_v)->LOCK); \
}


//==========================================================
// Public API.
//

cf_vector *
cf_vector_create(uint32_t ele_sz, uint32_t count, uint32_t flags)
{
	cf_vector *v = cf_malloc(sizeof(cf_vector));

	if (! v) {
		return NULL;
	}

	if (cf_vector_init(v, ele_sz, count,
			flags | VECTOR_FLAG_FREE_SELF) != 0) {
		cf_free(v);
		return NULL;
	}

	return v;
}

int
cf_vector_init(cf_vector *v, uint32_t ele_sz, uint32_t count, uint32_t flags)
{
	uint8_t *buf;

	if (count) {
		if (! (buf = cf_malloc(count * ele_sz))) {
			return -1;
		}
	}
	else {
		buf = 0;
	}

	cf_vector_init_with_buf(v, ele_sz, count, buf,
			flags | VECTOR_FLAG_FREE_VECTOR);

	return 0;
}

void
cf_vector_init_with_buf(cf_vector *v, uint32_t ele_sz, uint32_t count,
		uint8_t *buf, uint32_t flags)
{
	v->ele_sz = ele_sz;
	v->flags = flags;
	v->alloc_cnt = count;
	v->count = 0;
	v->vector = buf;

	if ((flags & VECTOR_FLAG_INITZERO) && v->vector) {
		memset(v->vector, 0, count * ele_sz);
	}

	if (flags & VECTOR_FLAG_BIGLOCK) {
		pthread_mutex_init(&v->LOCK, 0);
	}
}

// Deprecated - use cf_vector_init_with_buf().
void
cf_vector_init_smalloc(cf_vector *v, uint32_t ele_sz, uint8_t *sbuf,
		uint32_t sbuf_sz, uint32_t flags)
{
	cf_vector_init_with_buf(v, ele_sz, sbuf_sz / ele_sz, sbuf, flags);
}

void
cf_vector_destroy(cf_vector *v)
{
	if (v->flags & VECTOR_FLAG_BIGLOCK) {
		pthread_mutex_destroy(&v->LOCK);
	}

	if (v->vector && (v->flags & VECTOR_FLAG_FREE_VECTOR)) {
		cf_free(v->vector);
	}

	if (v->flags & VECTOR_FLAG_FREE_SELF) {
		cf_free(v);
	}
}

int
cf_vector_set(cf_vector *v, uint32_t idx, const void *val)
{
	VECTOR_LOCK(v);

	if (idx >= v->alloc_cnt) {
		VECTOR_UNLOCK(v);
		return -1;
	}

	memcpy(v->vector + (idx * v->ele_sz), val, v->ele_sz);

	if (idx >= v->count) {
		v->count = idx + 1;
	}

	VECTOR_UNLOCK(v);

	return 0;
}

int
cf_vector_append_lockfree(cf_vector *v, const void *val)
{
	if (v->count >= v->alloc_cnt && ! vector_resize(v, v->count * 2)) {
		return -1;
	}

	memcpy(v->vector + (v->count * v->ele_sz), val, v->ele_sz);
	v->count++;

	return 0;
}

int
cf_vector_append(cf_vector *v, const void *val)
{
	VECTOR_LOCK(v);

	int ret = cf_vector_append_lockfree(v, val);

	VECTOR_UNLOCK(v);

	return ret;
}

int
cf_vector_append_unique(cf_vector *v, const void *val)
{
	VECTOR_LOCK(v);

	uint8_t	*p = v->vector;
	uint32_t ele_sz = v->ele_sz;

	for (uint32_t i = 0; i < v->count; i++) {
		if (memcmp(val, p, ele_sz) == 0) {
			VECTOR_UNLOCK(v);
			return 0;
		}

		p += ele_sz;
	}

	int rv = cf_vector_append_lockfree(v, val);

	VECTOR_UNLOCK(v);

	return rv;
}

int
cf_vector_pop(cf_vector *v, void *val)
{
	VECTOR_LOCK(v);

	if (v->count == 0) {
		VECTOR_UNLOCK(v);
		return -1;
	}

	v->count--;
	memcpy(val, v->vector + v->count * v->ele_sz, v->ele_sz);

	VECTOR_UNLOCK(v);

	return 0;
}

int
cf_vector_get(const cf_vector *v, uint32_t idx, void *val)
{
	VECTOR_LOCK(v);

	if (idx >= v->alloc_cnt) {
		VECTOR_UNLOCK(v);
		return -1;
	}

	memcpy(val, v->vector + (idx * v->ele_sz), v->ele_sz);

	VECTOR_UNLOCK(v);

	return 0;
}

bool
cf_vector_get_sized(const cf_vector *v, uint32_t idx, void *val, uint32_t sz)
{
	if (v->ele_sz <= sz) {
		return cf_vector_get(v, idx, val) == 0;
	}

	uint8_t buf[v->ele_sz];

	if (cf_vector_get(v, idx, &buf) != 0) {
		return false;
	}

	return true;
}

void *
cf_vector_getp(const cf_vector *v, uint32_t idx)
{
	VECTOR_LOCK(v);

	if (idx >= v->alloc_cnt) {
		VECTOR_UNLOCK(v);
		return NULL;
	}

	void *ele = v->vector + (idx * v->ele_sz);

	VECTOR_UNLOCK(v);

	return ele;
}

void *
cf_vector_getp_vlock(const cf_vector *v, uint32_t idx, pthread_mutex_t **vlock)
{
	if (! v->flags & VECTOR_FLAG_BIGLOCK) {
		return NULL;
	}

	VECTOR_LOCK(v);

	if (idx >= v->alloc_cnt) {
		VECTOR_UNLOCK(v);
		return NULL;
	}

	*vlock = &((cf_vector *)v)->LOCK;

	return v->vector + (idx * v->ele_sz);
}

int
cf_vector_delete(cf_vector *v, uint32_t idx)
{
	VECTOR_LOCK(v);

	if (idx >= v->count) {
		VECTOR_UNLOCK(v);
		return -1;
	}

	if (idx != v->count - 1) {
		memmove(v->vector + (idx * v->ele_sz),
				v->vector + ((idx + 1) * v->ele_sz),
				(v->count - (idx + 1)) * v->ele_sz );
	}

	v->count--;

	VECTOR_UNLOCK(v);

	return 0;
}

int
cf_vector_delete_range(cf_vector *v, uint32_t start, uint32_t end)
{

	if (start >= end) {
		return -1;
	}

	VECTOR_LOCK(v);

	if (start >= v->count || end > v->count) {
		VECTOR_UNLOCK(v);
		return -1;
	}

	// Copy down if not at end.
	if (end != v->count) {
		memmove( v->vector + (start * v->ele_sz),
				v->vector + ((end) * v->ele_sz),
			    (v->count - end) * v->ele_sz );

	}

	v->count -= end - start;

	VECTOR_UNLOCK(v);

	return 0;
}

void
cf_vector_clear(cf_vector *v)
{
	VECTOR_LOCK(v);

	v->count = 0;

	if (v->flags & VECTOR_FLAG_INITZERO) {
		memset(v->vector, 0, v->alloc_cnt * v->ele_sz);
	}

	VECTOR_UNLOCK(v);
}

void
cf_vector_compact(cf_vector *v)
{
	VECTOR_LOCK(v);

	if (v->alloc_cnt && (v->count != v->alloc_cnt)) {
		v->vector = cf_realloc(v->vector, v->count * v->alloc_cnt);
		v->alloc_cnt = v->count;
	}

	VECTOR_UNLOCK(v);
}


//==========================================================
// Local helpers - variable key size public API.
//

static bool
vector_resize(cf_vector *v, uint32_t new_count)
{
	if (v->flags & VECTOR_FLAG_BIGRESIZE) {
		if (new_count < 50) {
			new_count = 50;
		}
	}
	else if (new_count == 0) {
		new_count = 2;
	}

	uint8_t *p;

	if (v->vector == 0 || ! (v->flags & VECTOR_FLAG_FREE_VECTOR)) {
		p = cf_malloc(new_count * v->ele_sz);

		if (! p)	{
			return false;
		}

		if (v->vector) {
			memcpy(p, v->vector, v->alloc_cnt * v->ele_sz);
			v->flags |= VECTOR_FLAG_FREE_VECTOR;
		}
	}
	else {
		p = cf_realloc(v->vector, new_count * v->ele_sz);
	}

	if (! p) {
		return false;
	}

	v->vector = p;

	if (v->flags & VECTOR_FLAG_INITZERO) {
		memset(v->vector + v->alloc_cnt * v->ele_sz, 0,
				(new_count - v->alloc_cnt) * v->ele_sz);
	}

	v->alloc_cnt = new_count;

	return true;
}
