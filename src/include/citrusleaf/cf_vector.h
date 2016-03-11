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
#pragma once

/*
 * A general purpose vector
 * Uses locks, so only moderately fast
 * If you need to deal with sparse data, really sparse data,
 * use a hash table. This assumes that packed data is a good idea.
 * Does the fairly trivial realloc thing for extension, so 
 * and you can keep adding cool things to it
 */

 
#include <pthread.h>
#include <stdint.h>

#include <citrusleaf/cf_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define VECTOR_ELEM_SZ(_v) ( _h->value_len )

/**
 * support multithreaded access with a single big lock
 */
#define VECTOR_FLAG_BIGLOCK 0x01

/**
 * internally init the vector objects to 0
 */
#define VECTOR_FLAG_INITZERO 0x02

/**
 * appends will be common - speculatively allocate extra memory
 */
#define VECTOR_FLAG_BIGRESIZE 0x04

/**
 * indicate that a delete should be done during the reduction
 */
#define VECTOR_REDUCE_DELETE (1)


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct cf_vector_s cf_vector;

struct cf_vector_s {
	uint32_t 		value_len;
	uint 			flags;
	uint 			alloc_len; // number of elements currently allocated
	uint 			len;       // number of elements in table, largest element set
	uint8_t *		vector;
	bool			stack_struct;
	bool			stack_vector;
    pthread_mutex_t LOCK;           // the mutex lock
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Create a vector with malloc for handing around
 */
cf_vector * cf_vector_create(uint32_t value_len, uint32_t init_sz, uint flags);

/**
 * create a stack vector, but with an allocated internal-vector-bit
 */
int cf_vector_init(cf_vector *v, uint32_t value_len, uint32_t init_sz, uint flags);

void cf_vector_init_smalloc(cf_vector *v, uint32_t value_len, uint8_t *sbuf, int sbuf_sz, uint flags);

/**
 * Place a value into the vector
 * Value will be copied into the vector
 */
extern int cf_vector_get(cf_vector *v, uint32_t index, void *value);

/**
 * Retrieve a value from the vector
 */
extern int cf_vector_set(cf_vector *v, uint32_t index, void *value);

/**
 * this is very dangerous if it's a multithreaded vector. Use _vlock if multithrad.
 */
extern void * cf_vector_getp(cf_vector *v, uint32_t index);
extern void * cf_vector_getp_vlock(cf_vector *v, uint32_t index, pthread_mutex_t **vlock);
extern int cf_vector_append(cf_vector *v, void *value);

/**
 * Adds a an element to the end, only if it doesn't exist already
 * uses a bit-by-bit compare, thus is O(N) against the current length
 * of the vector
 */
extern int cf_vector_append_unique(cf_vector *v, void *value);

/**
 * Deletes an element by moving all the remaining elements down by one
 */
extern int cf_vector_delete(cf_vector *v, uint32_t index);

/**
 * Delete a range in the vector. Inclusive. Thus:
 *   a vector with len 5, you could delete start=0, end=3, leaving one element at the beginning (slot 0)
 *   don't set start and end the same, that's a single element delete, use vector_delete instead
 *   (or change the code to support that!)
 *   returns -1 on bad ranges
 */
extern int cf_vector_delete_range(cf_vector *v, uint32_t start_index, uint32_t end_index);

/**
 * There may be more allocated than you need. Fix that.
 */
extern void cf_vector_compact(cf_vector *v);

/**
 * Destroy the entire hash - all memory will be freed
 */
extern void cf_vector_destroy(cf_vector *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Get the number of elements currently in the vector
 */
static inline uint32_t cf_vector_size(cf_vector *v) {
	return(v->len);	
}


/**
 * nice wrapper functions
 * very common vector types are pointers, and integers
 */
static inline cf_vector * cf_vector_pointer_create(uint32_t init_sz, uint32_t flags) {
	return(cf_vector_create(sizeof(void *), init_sz, flags));
}

static inline int cf_vector_pointer_init(cf_vector *v, uint32_t init_sz, uint32_t flags) {
	return(cf_vector_init(v, sizeof(void *), init_sz, flags));
}

static inline int cf_vector_pointer_set(cf_vector *v, uint32_t index, void *p) {
	return(cf_vector_set(v, index, &p));
}

static inline void * cf_vector_pointer_get(cf_vector *v, uint32_t index) {
	void *p;
	cf_vector_get(v, index, &p);
	return(p);
}

static inline int cf_vector_pointer_append(cf_vector *v, void *p) {
	return(cf_vector_append(v, &p));
}

/**
 * integer vectors!
 */

static inline cf_vector * cf_vector_integer_create(uint32_t init_sz, uint32_t flags) {
	return(cf_vector_create(sizeof(int), init_sz, flags));
}

static inline int cf_vector_integer_init(cf_vector *v, uint32_t init_sz, uint32_t flags) {
	return(cf_vector_init(v, sizeof(int), init_sz, flags));
}

static inline int cf_vector_integer_set(cf_vector *v, uint32_t index, int i) {
	return(cf_vector_set(v, index, &i));
}

static inline int cf_vector_integer_get(cf_vector *v, uint32_t index) {
	int i;
	cf_vector_get(v, index, &i);
	return(i);
}

static inline int cf_vector_integer_append(cf_vector *v, int i) {
	return(cf_vector_append(v, &i));
}

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cf_vector_define(__x, __value_len, __flags) \
	uint8_t cf_vector##__x[1024]; cf_vector __x; cf_vector_init_smalloc(&__x, __value_len, cf_vector##__x, sizeof(cf_vector##__x), __flags);

#define cf_vector_reset( __v ) (__v)->len = 0; if ( (__v)->flags & VECTOR_FLAG_INITZERO) memset( (__v)->vector, 0, (__v)->alloc_len * (__v)->value_len);

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
