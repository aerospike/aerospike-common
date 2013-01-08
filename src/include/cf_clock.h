/*
 *      cf_clock.h
 *
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once
#include "cf_atomic.h"
#include "cf_types.h"
#include <pthread.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef uint64_t cf_clock;
typedef cf_atomic64 cf_atomic_clock;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_clock cf_getms();
cf_clock cf_getmicros();
cf_clock cf_getus();
cf_clock cf_clock_getabsolute();
cf_clock cf_get_seconds();
cf_clock cf_secs_since_clepoch();

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

static inline cf_clock CF_TIMESPEC_TO_MS_P( struct timespec *ts ) {
    uint64_t r1 = ts->tv_nsec;
    r1 /= 1000000;
    uint64_t r2 = ts->tv_sec;
    r2 *= 1000;
    return( r1 + r2 );
}

static inline cf_clock CF_TIMESPEC_TO_MS( struct timespec ts ) {
    uint64_t r1 = ts.tv_nsec;
    r1 /= 1000000;
    uint64_t r2 = ts.tv_sec;
    r2 *= 1000;
    return ( r1 + r2 );
}

static inline cf_clock CF_TIMESPEC_TO_US( struct timespec ts ) {
    uint64_t r1 = ts.tv_nsec;
    r1 /= 1000;
    uint64_t r2 = ts.tv_sec;
    r2 *= 1000000;
    return ( r1 + r2 );
}

static inline void CF_TIMESPEC_ADD_MS(struct timespec *ts, uint ms) {
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (ms % 1000) * 1000000;
    if (ts->tv_nsec > 1000000000) {
        ts->tv_sec ++;
        ts->tv_nsec -= 1000000000;
    }
}
