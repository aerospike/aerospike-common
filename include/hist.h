/*
 *  Citrusleaf Foundation
 *  include/hist.h - timer functionality
 *
 *  Copyright 2009 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once


#include <inttypes.h>

/* SYNOPSIS
 * For timing things, you want to know this histogram of what took how much time.
 * So have an interface where you create a histogram object, can dump a histogram object,
 * and can "start" / "stop" a given timer and add it to the histogram - multithread safe,
 * of course, because WE'RE CITRUSLEAF
 */

#define N_COUNTS 64

// #define USE_CLOCK

#define USE_GETCYCLES

//
// The counts are powers of two. count[0]
// count[0] is 1024 * 1024 a second
// count[13] is about a millisecond (1/1024 second)
// count[25] is a second


typedef struct histogram_counts_s {
	uint64_t count[N_COUNTS];
} histogram_counts;

typedef struct histogram_s {
	char name[64];
	cf_atomic_int n_counts;
	cf_atomic_int count[N_COUNTS];
} histogram;

typedef struct histogram_measure_s {
#ifdef USE_CLOCK	
	struct timespec start;
#endif
#ifdef USE_GETCYCLES
	uint64_t start;
#endif	
} histogram_measure;


extern histogram * histogram_create(char *name);
extern void histogram_dump( histogram *h );  // for debugging, dumps to stderr
extern void histogram_start( histogram *h, histogram_measure *hm);
extern void histogram_stop(histogram *h, histogram_measure *hm);
extern void histogram_get_counts(histogram *h, histogram_counts *hc);

// this is probably the same as the processor specific call, see if it works the same...

static inline uint64_t
hist_getcycles()
{
    int64_t c;

    __asm__ __volatile__ ("rdtsc" : "=A"(c) :: "memory");
    return(c);
}




