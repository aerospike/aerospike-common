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


// I hate those freeky little 't' things. Looks ugly.
// Add this if not a part of the CF framework
// typedef uint32_t uint32;
// typedef int32_t int32;
// typedef uint8_t byte;

#define BB_ERR_FOUND -4
#define BB_ERR_NOTFOUND -3
#define BB_ERR_BUFSZ -2
#define BB_ERR -1
#define BB_OK 0


/*
 * A generic call for hash functions the user can create
 */
typedef uint32 (*bbhash_hash_fn) (void *value, uint32 value_len);

/*
** Typedef for a "reduce" fuction that is called on every node
** (Note about return value: some kinds of reduces can manipulate the hash table,
**  allowing deletion. See the particulars of the reduce call.)
*/
typedef int (*bbhash_reduce_fn) (void *key, uint32 keylen, void *data, uint32 datalen, void *udata);

// Simple (and slow) element is when
// everything is variable (although a very nicely packed structure for 32 or 64
typedef struct bbhash_elem_s {
	struct bbhash_elem_s *next;
	uint32 key_len;
	uint32 value_len;
	void *key;
	void *value;
} bbhash_elem;



typedef struct bbhash_s {
	uint elements;
	uint32 key_len;
	uint32 value_len;
	uint flags;
	bbhash_hash_fn	h_fn;
	
	uint table_len; // number of elements currently in the table
	void *table;
	pthread_mutex_t		biglock;
} bbhash;

#define BBHASH_CR_RESIZE 0x01   // support resizes (will sometimes hang for long periods)
#define BBHASH_CR_GRAB   0x02   // support 'grab' call (requires more memory)
#define BBHASH_CR_MT_BIGLOCK 0x04 // support multithreaded access with a single big lock
#define BBHASH_CR_MT_LOCKPOOL 0x08 // support multithreaded access with a pool of object loccks



/*
 * Create a hash table
 * Pass in the hash function (required)
 * the key length if static (if not static pass 0
 * the value length if static (if not static pass 0
 * The initial table size
 * a set of flags
 */

int
bbhash_create(bbhash **h, bbhash_hash_fn h_fn, uint32 key_len, uint32 value_len, uint32 sz, uint flags);

/* Place a value into the hash
 * Value will be copied into the hash
 */
int
bbhash_put(bbhash *h, void *key, uint32 key_len, void *value, uint32 value_len);

/* If you think you know how much space it will take, 
 * call with the buffer you want filled
 * If you're wrong about the space, you'll get a BUFSZ error, but the *value_len
 * will be filled in with the value you should have passed
 */
int
bbhash_get(bbhash *h, void *key, uint32 key_len, void *value, uint32 *value_len);

/* Does a get and delete at the same time so you can make sure only one person
 * gets what was inserted
 */
int
bbhash_get_and_delete(bbhash *h, void *key, uint32 key_len, void *value, uint32 *value_len);


/*
** Got a key you want removed - this is the function to call
*/
int
bbhash_delete(bbhash *h, void *key, uint32 key_len);

/*
** Get the number of elements currently in the hash
*/
uint32
bbhash_get_size(bbhash *h);

/*
 * An interesting idea: readv / writev for these functions?
 */

/* Find / get a value from the hash
 * But take the reference count on the object; must be returned with the
 * return call
 */
int
bbhash_grab(bbhash *h, void *key, uint32 key_len, void **value, uint32 *value_len);

/* Return a value that has been gotten
 */
int
bbhash_return(bbhash *h, void *value);

/*
** Map/Reduce pattern - call the callback on every element in the hash
** Warning: the entire traversal can hold the lock in the 'biglock' case,
** so make the reduce_fn lightweight! Consider queuing or soemthing if you
** want to do something fancy
*/
int
bbhash_reduce(bbhash *h, bbhash_reduce_fn reduce_fn, void *udata);

/*
** Map/Reduce pattern - call the callback on every element in the hash
** This instance allows deletion of hash elements during the reduce:
** return -1 to cause the deletion of the element visisted
*/
#define BBHASH_REDUCE_DELETE (1) // return this from the reduce when you want to delete

int
bbhash_reduce_delete(bbhash *h, bbhash_reduce_fn reduce_fn, void *udata);


/*
 * Destroy the entire hash - all memory will be freed
 */
void
bbhash_destroy(bbhash *h);

