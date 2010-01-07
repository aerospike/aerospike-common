/*
 * A general purpose hashtable implementation
 * Uses locks, so only moderately fast
 * Just, hopefully, the last hash table you'll ever need
 * And you can keep adding cool things to it
 * Copywrite 2008 Brian Bulkowski
 * All rights reserved
 */

#pragma once
 
#include <inttypes.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#include "cf.h"

#define RCHASH_ERR_FOUND -4
#define RCHASH_ERR_NOTFOUND -3
#define RCHASH_ERR_BUFSZ -2
#define RCHASH_ERR -1
#define RCHASH_OK 0

#define RCHASH_CR_RESIZE 0x01   // support resizes (will sometimes hang for long periods)
#define RCHASH_CR_GRAB   0x02   // support 'grab' call (requires more memory)
#define RCHASH_CR_MT_BIGLOCK 0x04 // support multithreaded access with a single big lock
#define RCHASH_CR_MT_MANYLOCK 0x08 // support multithreaded access with a pool of object loccks


/*
 * A generic call for hash functions the user can create
 */
typedef uint32_t (*rchash_hash_fn) (void *value, uint32_t value_len);

/*
** Typedef for a "reduce" fuction that is called on every node
** (Note about return value: some kinds of reduces can manipulate the hash table,
**  allowing deletion. See the particulars of the reduce call.)
*/
typedef int (*rchash_reduce_fn) (void *key, uint32_t keylen, void *object, void *udata);

/*
** need a destructor for the object. It's a little complicated.
*/

typedef void (*rchash_destructor_fn) (void *object);

// Simple (and slow) element is when
// everything is variable (although a very nicely packed structure for 32 or 64
typedef struct rchash_elem_v_s {
	struct rchash_elem_v_s *next;
	void *object; // this is a reference counted object
	uint32_t key_len;
	void *key; 
} rchash_elem_v;

// When the key size is fixed, life can be simpler
typedef struct rchash_elem_f_s {
	struct rchash_elem_f_s *next;
	void *object; // this is a reference counted object
	uint8_t key[];
} rchash_elem_f;


//
// An interesting tradeoff regarding 'get_size'
// In the case of many-locks, there's no real size at any given instant,
// because the hash is parallelized. Yet, the overhead of creating
// an atomic for the elements is silly.
// Thus, when 'manylock', the elements field is not useful because
// its not protected by a lock - it will *typically* get boned up.
// Thus, get_size has to slowly troop through the hashset
// This seems reasonable because 'get_size' with many lock can't be important,
// since it's always an estimate anyway.
//


typedef struct rchash_s {
	uint32_t elements;   // INVALID IF MANYLOCK
	uint32_t key_len;    // if key_len == 0, then use the variable size functions
	uint flags;
	rchash_hash_fn	h_fn;
	rchash_destructor_fn d_fn;
	
	uint table_len; // number of elements currently in the table
	void *table;
	pthread_mutex_t		biglock;
	pthread_mutex_t		*lock_table;
} rchash;

#define RCHASH_CR_RESIZE 0x01   // support resizes (will sometimes hang for long periods)
#define RCHASH_CR_MT_BIGLOCK 0x04 // support multithreaded access with a single big lock
#define RCHASH_CR_MT_LOCKPOOL 0x08 // support multithreaded access with a pool of object loccks

#define RCHASH_REDUCE_DELETE (1)	// indicate that a delete should be done during reduction

/*
 * Create a hash table
 * Pass in the hash function (required)
 * the key length if static (if not static pass 0
 * the value length if static (if not static pass 0
 * The initial table size
 * a set of flags
 */

int
rchash_create(rchash **h, rchash_hash_fn h_fn, rchash_destructor_fn d_fn, uint32_t key_len, uint32_t sz, uint flags);

/* Place a value into the hash
 * Value will be copied into the hash
 */
int
rchash_put(rchash *h, void *key, uint32_t key_len, void *value);

int
rchash_put_unique(rchash *h, void *key, uint32_t key_len, void *value);

/* If you think you know how much space it will take, 
 * call with the buffer you want filled
 * If you're wrong about the space, you'll get a BUFSZ error, but the *value_len
 * will be filled in with the value you should have passed
 */
int
rchash_get(rchash *h, void *key, uint32_t key_len, void **object);

/*
** Got a key you want removed - this is the function to call
*/
int
rchash_delete(rchash *h, void *key, uint32_t key_len);

/*
** Get the number of elements currently in the hash
*/
uint32_t
rchash_get_size(rchash *h);


/*
** Map/Reduce pattern - call the callback on every element in the hash
** Warning: the entire traversal can hold the lock in the 'biglock' case,
** so make the reduce_fn lightweight! Consider queuing or soemthing if you
** want to do something fancy
*/
void
rchash_reduce(rchash *h, rchash_reduce_fn reduce_fn, void *udata);

/*
** Map/Reduce pattern - call the callback on every element in the hash
** This instance allows deletion of hash elements during the reduce:
** return -1 to cause the deletion of the element visisted
*/
void
rchash_reduce_delete(rchash *h, rchash_reduce_fn reduce_fn, void *udata);


/*
 * Destroy the entire hash - all memory will be freed
 */
void
rchash_destroy(rchash *h);

