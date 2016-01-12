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

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <citrusleaf/cf_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define SHASH_ERR_FOUND -4
#define SHASH_ERR_NOTFOUND -3
#define SHASH_ERR_BUFSZ -2
#define SHASH_ERR -1
#define SHASH_OK 0

/**
 * support resizes (will sometimes hang for long periods)
 */
#define SHASH_CR_RESIZE 0x01

/**
 * support 'grab' call (requires more memory)
 */
#define SHASH_CR_GRAB   0x02

/**
 * support multithreaded access with a single big lock
 */
#define SHASH_CR_MT_BIGLOCK 0x04

/**
 * support multithreaded access with a pool of object loccks
 */
#define SHASH_CR_MT_MANYLOCK 0x08

/**
 * Do not track memory allocations in this hash table.
 * (Used only when creating the hash table tracking memory allocations....)
 */
#define SHASH_CR_UNTRACKED 0x10

/**
 * indicate that a delete should be done during the reduction
 */
#define SHASH_REDUCE_DELETE (1)

/******************************************************************************
 * TYPES
 ******************************************************************************/

/**
 * A generic call for hash functions the user can create
 */
typedef uint32_t (*shash_hash_fn) (void *key);

/**
 * Type for a function to be called under the hash table locks to atomically update a hash table entry.
 * The old value is the current value of the key, or NULL if non-existent.
 * The new value is allocated by the caller.
 * User data can be anything.
 */
typedef void (*shash_update_fn) (void *key, void *value_old, void *value_new, void *udata);

/**
 * Typedef for a "reduce" fuction that is called on every node
 * (Note about return value: some kinds of reduces can manipulate the hash table,
 *  allowing deletion. See the particulars of the reduce call.)
 */
typedef int (*shash_reduce_fn) (void *key, void *data, void *udata);

/**
 * Simple (and slow) element is when
 * everything is variable (although a very nicely packed structure for 32 or 64
 */
struct shash_elem_s {
	struct 			shash_elem_s *next;
	bool			in_use;
	uint8_t			data[];   // key_len bytes of key, value_len bytes of value
};

typedef struct shash_elem_s shash_elem;

struct shash_s {
	uint 				elements; 		// INVALID in manylocks case - see notes under get_size
	uint32_t 			key_len;
	uint32_t 			value_len;
	uint 				flags;
	shash_hash_fn		h_fn;
	uint 				table_len; 		// number of elements currently in the table
	void *				table;
	pthread_mutex_t		biglock;
	pthread_mutex_t	*	lock_table;
};

typedef struct shash_s shash;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Create a hash table
 * Pass in the hash function (required)
 * the key length if static (if not static pass 0
 * the value length if static (if not static pass 0
 * The initial table size
 * a set of flags
 */
int shash_create(shash **h, shash_hash_fn h_fn, uint32_t key_len, uint32_t value_len, uint32_t sz, uint flags);

/**
 * Place a value into the hash
 * Value will be copied into the hash
 */
int shash_put(shash *h, void *key, void *value);

/**
 * Place a unique value into the hash
 * Value will be copied into the hash
 */
int shash_put_unique(shash *h, void *key, void *value);

/**
 * Place a duplicate value into the hash
 * Value will be copied into the hash
 */
int shash_put_duplicate(shash *h, void *key, void *value);

/**
 * call with the buffer you want filled; if you just want to check for
 * existence, call with value set to NULL
 */
int shash_get(shash *h, void *key, void *value);

/**
 * Returns the pointer to the internal item, and a locked-lock
 * which allows the touching of internal state. If non-lock hash table,
 * vlock param will be ignored
 *
 * Note that the vlock is passed back only when the return code is BB_OK.
 * In the case where nothing is found, no lock is held.
 * It might be better to do it the other way, but you can change it later if you want
 */
int shash_get_vlock(shash *h, void *key, void **value,pthread_mutex_t **vlock);

/**
 * Does a get and delete at the same time so you can make sure only one person
 * gets what was inserted
 */
int shash_get_and_delete(shash *h, void *key, void *value);

/**
 * Atomically update an entry in the hash table using a user-supplied update function and user data.
 * The update function performs the merge of the old and new values, with respect to the user data
 * and returns the new value.
 */
int shash_update(shash *h, void *key, void *value_old, void *value_new, shash_update_fn update_fn, void *udata);

/**
 * Got a key you want removed - this is the function to call
 */
int shash_delete(shash *h, void *key);

/**
 * Special function you can call when you already have the lock - such as
 * a vlock get
 */
int shash_delete_lockfree(shash *h, void *key);

/**
 * Get the number of elements currently in the hash
 */
uint32_t shash_get_size(shash *h);

/**
 * An interesting idea: readv / writev for these functions?
 */

/**
 * Find / get a value from the hash
 * But take the reference count on the object; must be returned with the
 * return call
 */
int shash_grab(shash *h, void *key, uint32_t key_len, void **value, uint32_t *value_len);

/**
 * Return a value that has been gotten
 */
int shash_return(shash *h, void *value);

/**
 * Map/Reduce pattern - call the callback on every element in the hash
 * Warning: the entire traversal can hold the lock in the 'biglock' case,
 * so make the reduce_fn lightweight! Consider queuing or soemthing if you
 * want to do something fancy
 */
int shash_reduce(shash *h, shash_reduce_fn reduce_fn, void *udata);

/**
 * Map/Reduce pattern - call the callback on every element in the hash
 * This instance allows deletion of hash elements during the reduce:
 * return -1 to cause the deletion of the element visisted
 */
int shash_reduce_delete(shash *h, shash_reduce_fn reduce_fn, void *udata);

/**
 * Delete all the data from the entire hash - complete cleanup
 */
void shash_deleteall_lockfree(shash *h);

/**
 * Delete all the data from the entire hash - after getting lock - complete cleanup
 */
void shash_deleteall(shash *h);

/**
 * Destroy the entire hash - all memory will be freed
 */
void shash_destroy(shash *h);

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define SHASH_ELEM_KEY_PTR(_h, _e) 	( (void *) _e->data )
#define SHASH_ELEM_VALUE_PTR(_h, _e) ( (void *) (_e->data + _h->key_len) )
#define SHASH_ELEM_SZ(_h) ( sizeof(shash_elem) + (_h->key_len) + (_h->value_len) )

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
