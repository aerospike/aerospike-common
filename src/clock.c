/*
 * Citrusleaf, 2008
 * No rights reserved.
 * Just kidding. Lots of rights. I mean,
 * ALL rights reserved
 */

#include <time.h>

#include "cf.h"

#define TIMESPEC_TO_MS( __ts )  ((__ts.tv_sec * 1000) + (__ts.tv_nsec / 1000000)) 

uint64_t
cf_getms() {
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts);
	return ( TIMESPEC_TO_MS (ts) );
}	


