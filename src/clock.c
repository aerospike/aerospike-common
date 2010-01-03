/*
 *  Citrusleaf Foundation
 *  src/clock.c - memory allocation framework
 *
 *  Copyright 2008-2009 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <time.h>
#include "cf.h"

inline static uint64_t
TIMESPEC_TO_MS_P( struct timespec *ts )
{
	uint64_t r1 = ts->tv_nsec;
	r1 /= 1000000;
	uint64_t r2 = ts->tv_sec;
	r2 *= 1000;
	return( r1 + r2 );
}

inline static uint64_t
TIMESPEC_TO_MS( struct timespec ts )
{
	uint64_t r1 = ts.tv_nsec;
	r1 /= 1000000;
	uint64_t r2 = ts.tv_sec;
	r2 *= 1000;
	return ( r1 + r2 );
}


// FIXME ought to be cf_clock_getvolatile or some shit
cf_clock
cf_getms() {
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( TIMESPEC_TO_MS (ts) );
}	

cf_clock
cf_clock_getabsolute() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return(TIMESPEC_TO_MS(ts));
}
