/* 
 * Copyright 2008-2016 Aerospike, Inc.
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

cf_clock cf_getns() {
#ifdef __APPLE__
	return mach_absolute_time();
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return CF_TIMESPEC_TO_NS(ts);
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
void cf_set_wait_timespec(int ms_wait, struct timespec* out)
{
#ifdef __APPLE__
    // This code is going to have slightly less resolution than the pure linux version.
	struct timeval now;
	gettimeofday(&now, NULL);
	out->tv_sec = now.tv_sec + (ms_wait / 1000);
	out->tv_nsec = now.tv_usec * 1000 + (ms_wait % 1000) * 1000 * 1000;
#else // linux
    clock_gettime(CLOCK_REALTIME, out);
    out->tv_sec += ms_wait / 1000;
    out->tv_nsec += (ms_wait % 1000) * 1000 * 1000;
#endif
	
	if (out->tv_nsec > (1000 * 1000 * 1000)) {
        out->tv_nsec -= 1000 * 1000 * 1000;
        out->tv_sec++;
    }
}

//
// Add delta to current time to produce absolute time.
//
void cf_clock_current_add(struct timespec* delta, struct timespec* out)
{
#ifdef __APPLE__
    // This code is going to have slightly less resolution than the pure linux version.
	struct timeval now;
	gettimeofday(&now, NULL);
	out->tv_sec = now.tv_sec + delta->tv_sec;
	out->tv_nsec = now.tv_usec * 1000 + delta->tv_nsec;
#else // linux
    clock_gettime(CLOCK_REALTIME, out);
    out->tv_sec += delta->tv_sec;
    out->tv_nsec += delta->tv_nsec;
#endif
	
    if (out->tv_nsec > (1000 * 1000 * 1000)) {
        out->tv_nsec -= 1000 * 1000 * 1000;
        out->tv_sec++;
    }
}
