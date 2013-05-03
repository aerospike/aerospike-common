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

#include <citrusleaf/cf_atomic.h>
#include <citrusleaf/cf_types.h>
#include <strings.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

static const char cf_LogTable256[] =
{
#define CF_LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    CF_LT(4), CF_LT(5), CF_LT(5), CF_LT(6), CF_LT(6), CF_LT(6), CF_LT(6),
    CF_LT(7), CF_LT(7), CF_LT(7), CF_LT(7), CF_LT(7), CF_LT(7), CF_LT(7), CF_LT(7)
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

extern int cf_bits_find_last_set(uint32_t c);
extern int cf_bits_find_last_set_64(uint64_t c);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

static inline uint32_t cf_roundup( uint32_t i, uint32_t modulus) {
    uint32_t t = i % modulus;
    if (t == 0) return(i);
    return(  i + (modulus - t ) );
}

static inline uint64_t cf_roundup_64( uint64_t i, uint32_t modulus) {
    uint64_t t = i % modulus;
    if (t == 0) return(i);
    return(  i + (modulus - t ) );
}

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cf_bits_find_first_set(__x) ffs(__x)
#define cf_bits_find_first_set_64(__x) ffsll(__x)

#define cf_max_uint32(__x, __y) ( (__x) > (__y) ? (__x) : (__y) )
#define cf_max(__x, __y) ( (__x) > (__y) ? (__x) : (__y) )

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif