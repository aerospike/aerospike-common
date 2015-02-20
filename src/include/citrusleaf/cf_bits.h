/* 
 * Copyright 2008-2015 Aerospike, Inc.
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
