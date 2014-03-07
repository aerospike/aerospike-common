/*
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
 */
#include <citrusleaf/cf_clock.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_clock cf_getms() {
#ifdef __APPLE__
	// mach_absolute_time() currently returns nano-seconds, but is not guaranteed to do so.
	// This code may have to be revised at a later date.
	return mach_absolute_time() / 1000000;
#else
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( CF_TIMESPEC_TO_MS (ts) );
#endif
}

cf_clock cf_getmicros() {
#ifdef __APPLE__
	return mach_absolute_time() / 1000;
#else
	struct timespec ts = { 0, 0};
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	uint64_t micro = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
	return(micro);
#endif
}

cf_clock cf_getus() {
#ifdef __APPLE__
	return mach_absolute_time() / 1000;
#else
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( CF_TIMESPEC_TO_US (ts) );
#endif
}

cf_clock cf_clock_getabsolute() {
#ifdef __APPLE__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return(CF_TIMESPEC_TO_MS(ts));
#endif
}

cf_clock cf_clock_getabsoluteus() {
#ifdef __APPLE__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return(CF_TIMESPEC_TO_US(ts));
#endif
}

cf_clock cf_get_seconds() {
#ifdef __APPLE__
	return mach_absolute_time() / 1000000000;
#else
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( ts.tv_sec );
#endif
}

cf_clock cf_secs_since_clepoch() {
#ifdef __APPLE__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec - CITRUSLEAF_EPOCH);
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (ts.tv_sec - CITRUSLEAF_EPOCH);
#endif
}

//
// Set timespec to wait in milliseconds
//
void cf_set_wait_timespec(int ms_wait, struct timespec* tp)
{
#ifdef __APPLE__
    // Use the cl generic functions defined in cf_clock.h. It is going to have
    // slightly less resolution than the pure linux version.
    uint64_t curms = cf_getms();
    tp->tv_sec = (curms + ms_wait) / 1000;
    tp->tv_nsec = (ms_wait % 1000) * 1000000;
#else // linux
    clock_gettime( CLOCK_REALTIME, tp);
    tp->tv_sec += ms_wait / 1000;
    tp->tv_nsec += (ms_wait % 1000) * 1000000;
    if (tp->tv_nsec > 1000000000) {
        tp->tv_nsec -= 1000000000;
        tp->tv_sec++;
    }
#endif
}
