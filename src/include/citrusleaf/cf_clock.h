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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
#include <time.h>
#include <bits/time.h>
#endif
    
#ifdef __APPLE__
#include <sys/time.h>
#endif
    
#ifdef CF_WINDOWS
#include <citrusleaf/cf_clock_win.h>
#endif

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
void cf_set_wait_timespec(int ms_wait, struct timespec* tp);

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

static inline uint32_t cf_clepoch_seconds() {
#ifdef __APPLE__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec - CITRUSLEAF_EPOCH);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint32_t)(ts.tv_sec - CITRUSLEAF_EPOCH);
#endif
}

// Special client-only conversion utility.
static inline uint32_t cf_server_void_time_to_ttl(uint32_t server_void_time) {
	// This is the server's flag indicating the record never expires...
	if (server_void_time == 0) {
		// ... converted to the new client-side convention for "never expires":
		return (uint32_t)-1;
	}

	uint32_t now = cf_clepoch_seconds();

	// Record may not have expired on server, but delay or clock differences may
	// cause it to look expired on client. (We give the record to the app anyway
	// to avoid internal cleanup complications.) Floor at 1, not 0, to avoid old
	// "never expires" interpretation.
	return server_void_time > now ? server_void_time - now : 1;
}

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
