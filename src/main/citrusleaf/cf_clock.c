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

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_clock cf_getms() {
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( CF_TIMESPEC_TO_MS (ts) );
}

cf_clock cf_getmicros() {
	struct timespec ts = { 0, 0};
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	uint64_t micro = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
	return(micro);
}

cf_clock cf_getus() {
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( CF_TIMESPEC_TO_US (ts) );
}	

cf_clock cf_clock_getabsolute() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return(CF_TIMESPEC_TO_MS(ts));
}

cf_clock cf_get_seconds() {
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( ts.tv_sec );
}

cf_clock cf_secs_since_clepoch() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (ts.tv_sec - CITRUSLEAF_EPOCH);
}

