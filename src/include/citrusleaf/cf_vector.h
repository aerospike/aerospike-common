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
#pragma once

//==========================================================
// Includes.
//

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <citrusleaf/cf_types.h>

#ifdef __cplusplus
extern "C" {
#endif


//==========================================================
// Constants & typedefs.
//

#define VECTOR_ELEM_SZ(_v) ( _v->ele_sz )

// Multithreaded access with a single big lock.
#define VECTOR_FLAG_BIGLOCK 0x01
// Init the vector objects to 0.
#define VECTOR_FLAG_INITZERO 0x02
// Appends will be common - speculatively allocate extra memory.
#define VECTOR_FLAG_BIGRESIZE 0x04

// Return to delete during reduce.
#define VECTOR_REDUCE_DELETE 1

typedef struct cf_vector_s {
	uint8_t *vector;
	uint32_t ele_sz;
	uint32_t alloc_cnt; // number of elements currently allocated
	uint32_t count;     // number of elements in table, largest element set
	uint32_t flags;
    pthread_mutex_t LOCK; // mutable
} cf_vector;


//==========================================================
// Public API.
//

cf_vector *cf_vector_create(uint32_t ele_sz, uint32_t count, uint32_t flags);
int cf_vector_init(cf_vector *v, uint32_t ele_sz, uint32_t count, uint32_t flags);
void cf_vector_init_with_buf(cf_vector *v, uint32_t ele_sz, uint32_t count, uint8_t *buf, uint32_t flags);

// Deprecated - use cf_vector_init_with_buf().
void cf_vector_init_smalloc(cf_vector *v, uint32_t ele_sz, uint8_t *sbuf, uint32_t sbuf_sz, uint32_t flags);

int cf_vector_get(const cf_vector *v, uint32_t idx, void *val);
bool cf_vector_get_sized(const cf_vector *v, uint32_t idx, void *val, uint32_t sz);
int cf_vector_set(cf_vector *v, uint32_t idx, const void *val);

void *cf_vector_getp(const cf_vector *v, uint32_t val);
void *cf_vector_getp_vlock(const cf_vector *v, uint32_t val, pthread_mutex_t **vlock);

int cf_vector_append(cf_vector *v, const void *val);
// Adds a an element to the end, only if it doesn't exist already. O(N).
int cf_vector_append_unique(cf_vector *v, const void *val);
int cf_vector_pop(cf_vector *v, void *val);

int cf_vector_delete(cf_vector *v, uint32_t val);
/**
 * Delete a range in the vector. Inclusive-Exclusive. Thus:
 *   a vector with len 5, you could delete 3 elements at indices (0,1, and 2)
 *   using start=0, end=3, leaving two elements at the beginning (slot 0)
 *   originally at indices 3 and 4.
 *   returns -1 on bad ranges
 */
int cf_vector_delete_range(cf_vector *v, uint32_t start, uint32_t end);
void cf_vector_clear(cf_vector *v);

// Realloc to minimal space needed.
void cf_vector_compact(cf_vector *v);

void cf_vector_destroy(cf_vector *v);

static inline uint32_t
cf_vector_size(const cf_vector *v)
{
	return v->count;
}

// Deprecated - use cf_vector_create().
static inline cf_vector *
cf_vector_pointer_create(uint32_t count, uint32_t flags)
{
	return cf_vector_create(sizeof(void *), count, flags);
}

// Deprecated - use cf_vector_init(v, sizeof(void *), ...).
static inline int
cf_vector_pointer_init(cf_vector *v, uint32_t count, uint32_t flags)
{
	return cf_vector_init(v, sizeof(void *), count, flags);
}

// Deprecated - use cf_vector_set_ptr().
static inline int
cf_vector_pointer_set(cf_vector *v, uint32_t idx, const void *val)
{
	return cf_vector_set(v, idx, &val);
}

// Deprecated - use cf_vector_get_ptr().
static inline void *
cf_vector_pointer_get(const cf_vector *v, uint32_t idx)
{
	void *p;

	if (! cf_vector_get_sized(v, idx, &p, sizeof(void *))) {
		return NULL;
	}

	return p;
}

// Deprecated - use cf_vector_append_ptr().
static inline int
cf_vector_pointer_append(cf_vector *v, const void *val)
{
	return cf_vector_append(v, &val);
}

static inline int
cf_vector_set_ptr(cf_vector *v, uint32_t idx, const void *val)
{
	return cf_vector_set(v, idx, &val);
}

static inline void *
cf_vector_get_ptr(const cf_vector *v, uint32_t idx)
{
	void *p;

	if (! cf_vector_get_sized(v, idx, &p, sizeof(void *))) {
		return NULL;
	}

	return p;
}

static inline int
cf_vector_append_ptr(cf_vector *v, const void *val)
{
	return cf_vector_append(v, &val);
}

static inline int
cf_vector_set_int(cf_vector *v, uint32_t idx, int val)
{
	return cf_vector_set(v, idx, &val);
}

static inline int
cf_vector_get_int(const cf_vector *v, uint32_t idx)
{
	int val;

	if (! cf_vector_get_sized(v, idx, &val, sizeof(int))) {
		return 0;
	}

	return val;
}

static inline int
cf_vector_append_int(cf_vector *v, int val)
{
	return cf_vector_append(v, &val);
}

static inline int
cf_vector_set_uint32(cf_vector *v, uint32_t idx, uint32_t val)
{
	return cf_vector_set(v, idx, &val);
}

static inline uint32_t
cf_vector_get_uint32(const cf_vector *v, uint32_t idx)
{
	uint32_t val;

	if (! cf_vector_get_sized(v, idx, &val, sizeof(uint32_t))) {
		return 0;
	}

	return val;
}

static inline int
cf_vector_append_uint32(cf_vector *v, uint32_t val)
{
	return cf_vector_append(v, &val);
}

// Removed - use cf_vector_inita or cf_vector_inits.
//#define cf_vector_define(__x, __value_len, __flags) \
//	uint8_t cf_vector##__x[1024]; cf_vector __x; cf_vector_init_smalloc(&__x, __value_len, cf_vector##__x, sizeof(cf_vector##__x), __flags);

// Removed - use cf_vector_clear()
//#define cf_vector_reset( __v ) (__v)->len = 0; if ( (__v)->flags & VECTOR_FLAG_INITZERO) memset( (__v)->vector, 0, (__v)->alloc_len * (__v)->ele_sz);

#define cf_vector_inita(_v, _ele_sz, _ele_cnt, _flags) \
		cf_vector_init_with_buf(_v, _ele_sz, _ele_cnt, alloca(_ele_sz * _ele_cnt), _flags);

#define cf_vector_inits(_v, _ele_sz, _ele_cnt, _flags) \
		uint8_t _v ## __mem[(_ele_sz) * (_ele_cnt)]; \
		cf_vector_init_with_buf(&_v, _ele_sz, _ele_cnt, _v ## __mem, _flags);


#ifdef __cplusplus
} // end extern "C"
#endif
