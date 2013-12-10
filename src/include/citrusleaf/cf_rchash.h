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

/*
 * A general purpose hashtable implementation
 * Uses locks, so only moderately fast
 * Just, hopefully, the last hash table you'll ever need
 * And you can keep adding cool things to it
 */

#include <citrusleaf/alloc.h>
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
 * support multithreaded access with a pool of object loccks
 */
#define CF_RCHASH_CR_MT_MANYLOCK 0x08

/**
 *don't calculate the size on every call, which makes 'getsize' expensive if you ever call it
 */
#define CF_RCHASH_CR_NOSIZE 0x10

/**
 * support resizes (will sometimes hang for long periods)
 */
#define CF_RCHASH_CR_RESIZE 0x01

/**
 * support multithreaded access with a single big lock
 */
#define CF_RCHASH_CR_MT_BIGLOCK 0x04

/**
 * support multithreaded access with a pool of object loccks
 */
#define CF_RCHASH_CR_MT_LOCKPOOL 0x08

/**
 * indicate that a delete should be done during reduction
 */
#define CF_RCHASH_REDUCE_DELETE (1)

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
 * Typedef for a "reduce" fuction that is called on every node
 * (Note about return value: some kinds of reduces can manipulate the hash table,
 *  allowing deletion. See the particulars of the reduce call.)
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
 * An interesting tradeoff regarding 'get_size'
 * In the case of many-locks, there's no real size at any given instant,
 * because the hash is parallelized. Yet, the overhead of creating
 * an atomic for the elements is silly.
 * Thus, when 'manylock', the elements field is not useful because
 * its not protected by a lock - it will *typically* get boned up.
 * Thus, get_size has to slowly troop through the hashset
 * This seems reasonable because 'get_size' with many lock can't be important,
 * since it's always an estimate anyway.
 */
struct cf_rchash_s {
	uint32_t 				elements;   		// INVALID IF MANYLOCK
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

/* If you think you know how much space it will take, 
 * call with the buffer you want filled
 * If you're wrong about the space, you'll get a BUFSZ error, but the *value_len
 * will be filled in with the value you should have passed
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
** so make the reduce_fn lightweight! Consider queuing or soemthing if you
** want to do something fancy
*/
void cf_rchash_reduce(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata);

/*
** Map/Reduce pattern - call the callback on every element in the hash
** This instance allows deletion of hash elements during the reduce:
** return -1 to cause the deletion of the element visisted
*/
void cf_rchash_reduce_delete(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata);


/*
 * Destroy the entire hash - all memory will be freed
 */
void cf_rchash_destroy(cf_rchash *h);

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
