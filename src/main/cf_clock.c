/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

/**
 *  clock.c - memory allocation framework
 */

#include "cf_clock.h"
#include <time.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

// FIXME ought to be cf_clock_getvolatile or some shit
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

