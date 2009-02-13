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

#define TIMESPEC_TO_MS( __ts )  ((__ts.tv_sec * 1000) + (__ts.tv_nsec / 1000000)) 


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
