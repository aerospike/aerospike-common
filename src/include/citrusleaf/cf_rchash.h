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
 * A general purpose hashtable implementation
 * Uses locks, so only moderately fast
 * Just, hopefully, the last hash table you'll ever need
 * And you can keep adding cool things to it
 */

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_atomic.h>
#include <citrusleaf/cf_types.h>
#include <inttypes.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define CF_RCHASH_ERR_FOUND -4
#define CF_RCHASH_ERR_NOTFOUND -3
#define CF_RCHASH_ERR_BUFSZ -2
#define CF_RCHASH_ERR -1
#define CF_RCHASH_OK 0
#define CF_RCHASH_REDUCE_DELETE 1

/**
 * support resizes (will sometimes hang for long periods)
 */
#define CF_RCHASH_CR_RESIZE 0x01

/**
 * support 'grab' call (requires more memory)
 */
#define CF_RCHASH_CR_GRAB   0x02

/**
 * support multithreaded access with a single big lock
 */
#define CF_RCHASH_CR_MT_BIGLOCK 0x04

/**
 * support multithreaded access with a pool of object locks
 */
#define CF_RCHASH_CR_MT_MANYLOCK 0x08

/**
 *don't calculate the size on every call, which makes 'getsize' expensive if you ever call it
 */
#define CF_RCHASH_CR_NOSIZE 0x10

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct cf_rchash_s cf_rchash;
typedef struct cf_rchash_elem_v_s cf_rchash_elem_v;
typedef struct cf_rchash_elem_f_s cf_rchash_elem_f;

/**
 * A generic call for hash functions the user can create
 */
typedef uint32_t (*cf_rchash_hash_fn) (void *value, uint32_t value_len);

/**
 * Typedef for a "reduce" function that is called on every node.
 * Note that the function's return value has the following effect
 * within cf_rchash_reduce():
 *     CF_RCHASH_OK (0) - continue iterating.
 *     CF_RCHASH_REDUCE_DELETE (1) - delete the current node.
 *     Anything else (e.g. CF_RCHASH_ERR) - stop iterating.
 */
typedef int (*cf_rchash_reduce_fn) (void *key, uint32_t keylen, void *object, void *udata);

/**
 * need a destructor for the object.
 *
 * Importantly - since the hash table knows about the reference-counted nature of 
 * the stored objects, a 'delete' will have to decrement the reference count, thus
 * likely will need to call a function to clean the internals of the object.
 * this destructor should not free the object, and will be called only when the reference
 * count is 0. If you don't have any internal state in the object, you can pass NULL
 * as the destructor.
 *
 * This function is also called if there's a 'reduce' that returns 'delete'
 */
typedef void (*cf_rchash_destructor_fn) (void *object);

/**
 * Simple (and slow) element is when
 * everything is variable (although a very nicely packed structure for 32 or 64
 */
struct cf_rchash_elem_v_s {
	cf_rchash_elem_v *	next;
	void *				object; // this is a reference counted object
	uint32_t 			key_len;
	void *				key; 
};

/**
 * When the key size is fixed, life can be simpler
 */
struct cf_rchash_elem_f_s {
	cf_rchash_elem_f * 	next;
	void *				object; // this is a reference counted object
	uint8_t 			key[];
};


/**
 * Private data.
 */
struct cf_rchash_s {
	cf_atomic32 			elements;
	uint32_t 				key_len;    		// if key_len == 0, then use the variable size functions
	uint 					flags;
	cf_rchash_hash_fn		h_fn;
	cf_rchash_destructor_fn d_fn;
	uint 					table_len; 			// number of elements currently in the table
	void *					table;
	pthread_mutex_t			biglock;
	int						lock_table_len;
	int						buckets_per_lock; 	// precompute: buckets / locks
	pthread_mutex_t * 		lock_table;
};


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/*
 * Create a hash table
 * Pass in the hash function (required)
 * the key length if static (if not static pass 0
 * the value length if static (if not static pass 0
 * The initial table size
 * a set of flags
 */
int cf_rchash_create(cf_rchash **h, cf_rchash_hash_fn h_fn, cf_rchash_destructor_fn d_fn, uint32_t key_len, uint32_t sz, uint flags);

int cf_rchash_set_nlocks(cf_rchash *h, int n_locks);


/* Place a value into the hash
 * Value will be copied into the hash
 */
int cf_rchash_put(cf_rchash *h, void *key, uint32_t key_len, void *value);

int cf_rchash_put_unique(cf_rchash *h, void *key, uint32_t key_len, void *value);

/* If the key is found and a value pointer is returned in "object", the value's
 * ref count will be incremented. The caller must release this when finished.
 * If the "object" parameter is null, the function simply checks for key
 * existence, and if the key exists the value's ref count is not increased.
 */
int cf_rchash_get(cf_rchash *h, void *key, uint32_t key_len, void **object);

/*
** Got a key you want removed - this is the function to call
*/
int cf_rchash_delete(cf_rchash *h, void *key, uint32_t key_len);

/*
** Get the number of elements currently in the hash
*/
uint32_t cf_rchash_get_size(cf_rchash *h);

/*
** Map/Reduce pattern - call the callback on every element in the hash
** Warning: the entire traversal can hold the lock in the 'biglock' case,
** so make the reduce_fn lightweight! Consider queuing or something if you
** want to do something fancy
*/
int cf_rchash_reduce(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata);

/*
 * Destroy the entire hash - all memory will be freed
 */
void cf_rchash_destroy(cf_rchash *h);

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
