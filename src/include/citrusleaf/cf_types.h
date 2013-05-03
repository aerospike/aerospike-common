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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>   
#include <asm/byteorder.h>


#ifndef CF_WINDOWS
/******************************************************************************
 * LINUX
 *****************************************************************************/

#include <stdbool.h>    // A real pity that Linux requires this for bool, true & false:
#include <alloca.h>     // Use alloca() instead of variable-sized stack arrays for non-gcc portability.

#else // CF_WINDOWS
/******************************************************************************
 * WINDOWS
 *****************************************************************************/

#include <malloc.h>     // for alloca()

#endif // CF_WINDOWS



#ifdef __cplusplus
extern "C" {
#endif

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

struct cf_bytearray_t {
    uint64_t    sz;
    byte        data[];
};

typedef struct cf_bytearray_t cf_bytearray;

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif