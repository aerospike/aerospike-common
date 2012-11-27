/*
 *  Citrusleaf Foundation
 *  src/timer.c - timer functionality
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

#include "cf.h"

// #define DEBUG 1


int
bits_find_last_set(uint32_t v)
{

	int r;
	uint32_t t, tt;
	
	if ((tt = v >> 16))
		r = (t = tt >> 8) ? (24 + LogTable256[t]) : (16 + LogTable256[tt]);
	else
		r = (t = v >> 8) ? (8 + LogTable256[t]) : LogTable256[v];
	return (r);
}

int
bits_find_last_set_64(uint64_t v) 
{
	uint64_t t;
	if ((t = v >> 32))
		return( bits_find_last_set(t) + 32 );
	else
		return( bits_find_last_set(v) );
}

