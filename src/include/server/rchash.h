/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

/**
 * A general purpose hashtable implementation
 * Uses locks, so only moderately fast
 * Just, hopefully, the last hash table you'll ever need
 * And you can keep adding cool things to it
 */

#pragma once
#include "../cf_rchash.h"

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define RCHASH_ERR_FOUND -4
#define RCHASH_ERR_NOTFOUND -3
#define RCHASH_ERR_BUFSZ -2
#define RCHASH_ERR -1
#define RCHASH_OK 0

#define RCHASH_CR_RESIZE 0x01   // support resizes (will sometimes hang for long periods)
#define RCHASH_CR_GRAB   0x02   // support 'grab' call (requires more memory)
#define RCHASH_CR_MT_BIGLOCK 0x04 // support multithreaded access with a single big lock
#define RCHASH_CR_MT_MANYLOCK 0x08 // support multithreaded access with a pool of object loccks
#define RCHASH_CR_NOSIZE 0x10 // don't calculate the size on every call, which makes 'getsize' expensive if you ever call it

#define RCHASH_CR_RESIZE 0x01   // support resizes (will sometimes hang for long periods)
#define RCHASH_CR_MT_BIGLOCK 0x04 // support multithreaded access with a single big lock
#define RCHASH_CR_MT_LOCKPOOL 0x08 // support multithreaded access with a pool of object loccks

#define RCHASH_REDUCE_DELETE (1)    // indicate that a delete should be done during reduction


/******************************************************************************
 * TYPE ALIASES
 ******************************************************************************/

typedef struct cf_rchash_s rchash;
typedef struct cf_rchash_elem_v_s rchash_elem_v;
typedef struct cf_rchash_elem_f_s rchash_elem_f;
typedef uint32_t (*rchash_hash_fn) (void *value, uint32_t value_len);
typedef int (*rchash_reduce_fn) (void *key, uint32_t keylen, void *object, void *udata);
typedef void (*rchash_destructor_fn) (void *object);

/******************************************************************************
 * FUNCTION ALIASES
 ******************************************************************************/

#define rchash_create cf_rchash_create
#define rchash_set_nlocks cf_rchash_set_nlocks
#define rchash_put cf_rchash_put
#define rchash_put_unique cf_rchash_put_unique
#define rchash_get cf_rchash_get
#define rchash_delete cf_rchash_delete
#define rchash_get_size cf_rchash_get_size
#define rchash_reduce cf_rchash_reduce
#define rchash_reduce_delete cf_rchash_reduce_delete
#define rchash_destroy cf_rchash_destroy
#define rchash_dump cf_rchash_dump
