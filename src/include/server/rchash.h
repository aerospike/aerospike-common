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
