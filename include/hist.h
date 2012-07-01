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


#define HISTOGRAM_NAME_SIZE 128

#define N_SECS 60
#define N_MINS 59
#define N_HOURS 24
#define N_PCT 7

typedef struct histogram_counts_s {
	uint64_t count[N_COUNTS];
} histogram_counts;

// Define histogram structure, refer hist.c to see usage
// secs[N_SECS][0] is always the total number of transactions for that ticker/second i.e n_counts_pct 
// secs[N_SECS][1] is the number of transactions over 1 ms bucket
// secs[N_SECS][2] is the number of transactions over 2 ms bucket
// secs[N_SECS][3] is the number of transactions over 4 ms bucket 
// and so on....
// mins[N_MINS][N_PCT] and hours[N_HOURS][N_PCT] also follow the same structure.
 
typedef struct histogram_s {
	char name[HISTOGRAM_NAME_SIZE];
	cf_atomic_int n_counts;
	cf_atomic_int count[N_COUNTS];
	cf_atomic_int n_counts_pct;
	cf_atomic_int count_pct[N_COUNTS];
	cf_atomic_int secs[N_SECS][N_PCT];
	cf_atomic_int mins[N_MINS][N_PCT];
	cf_atomic_int hours[N_HOURS][N_PCT];
	
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
extern histogram * histogram_create_pct(char *name);
extern void histogram_clear(histogram *h);
extern void histogram_clear_pct(histogram *h);
extern void histogram_dump( histogram *h );  // for debugging, dumps to stderr
extern void histogram_start( histogram *h, histogram_measure *hm);
extern void histogram_stop(histogram *h, histogram_measure *hm);
extern void histogram_get_counts(histogram *h, histogram_counts *hc);

#ifdef USE_GETCYCLES
extern void histogram_insert_data_point(histogram *h, uint64_t start);
extern void histogram_insert_data_point_pct(histogram *h, uint64_t start);
#endif

// this is probably the same as the processor specific call, see if it works the same...

static inline uint64_t
hist_getcycles()
{
    int64_t c;

    __asm__ __volatile__ ("rdtsc" : "=A"(c) :: "memory");
    return(c);
}




/* SYNOPSIS
 * Linear histogram partitions the values into 20 buckets, < 5%, < 10%, < 15%, ..., < 95%, rest
 */

#define LINEAR_N_COUNTS 20

//
// The counts are linear.
// count[0] is the number in the first 5%
// count[1] is all data between 5% and 10%
// count[18] is all data between 90% and 95%
// count[19] is all data > 95%
//


typedef struct linear_histogram_counts_s {
	uint64_t count[LINEAR_N_COUNTS];
} linear_histogram_counts;

typedef struct linear_histogram_s {
	char name[HISTOGRAM_NAME_SIZE];
	uint64_t start;
	uint64_t offset20;
	cf_atomic_int n_counts;
	cf_atomic_int count[LINEAR_N_COUNTS];
} linear_histogram;

extern linear_histogram * linear_histogram_create(char *name, uint64_t start, uint64_t max_offset);
extern void linear_histogram_dump( linear_histogram *h );  // for debugging, dumps to stderr
extern void linear_histogram_get_counts(linear_histogram *h, linear_histogram_counts *hc);
extern void linear_histogram_insert_data_point(linear_histogram *h, uint64_t point);
extern size_t linear_histogram_get_index_for_pct(linear_histogram *h, size_t pct);

