/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

/**
 * SYNOPSIS
 * For timing things, you want to know this histogram of what took how much time.
 * So have an interface where you create a histogram object, can dump a histogram object,
 * and can "start" / "stop" a given timer and add it to the histogram - multithread safe,
 * of course.
 */

#pragma once
#include "cf_atomic.h"
#include "cf_bits.h"
#include "../cf_types.h"
#include <strings.h>
#include <string.h>
#include <inttypes.h>

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define CF_N_HIST_COUNTS 64

//
// The counts are powers of two. count[0]
// count[0] is 1024 * 1024 a second
// count[13] is about a millisecond (1/1024 second)
// count[25] is a second

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct cf_histogram_s cf_histogram;
typedef struct cf_histogram_counts_s cf_histogram_counts;

struct cf_histogram_counts_s {
	uint64_t 		count[CF_N_HIST_COUNTS];
};

struct cf_histogram_s {
	char 			name[64];
	cf_atomic_int 	n_counts;
	cf_atomic_int 	count[CF_N_HIST_COUNTS];
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_histogram * cf_histogram_create(char *name);
void cf_histogram_dump( cf_histogram *h );  // for debugging
void cf_histogram_get_counts(cf_histogram *h, cf_histogram_counts *hc);
void cf_histogram_insert_data_point(cf_histogram *h, uint64_t start);
