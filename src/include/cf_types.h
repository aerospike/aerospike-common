/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>   
#include <asm/byteorder.h>

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define CITRUSLEAF_EPOCH 1262304000
#define INVALID_FD (-1)
#define FALSE 0
#define TRUE 1

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef uint8_t byte;
typedef unsigned int uint;
typedef struct cf_bytearray_t cf_bytearray;

/**
 * cf_bytearray
 * An array of bytes
 * TO AVOID CONFUSION, always create a cf_bytearray as a reference counted
 * object! Always use them as reference counted objects! Always!!! 
 */
struct cf_bytearray_t {
    uint64_t    sz;
    byte        data[];
};

