/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

/*
 * A general purpose vector
 * Uses locks, so only moderately fast
 * If you need to deal with sparse data, really sparse data,
 * use a hash table. This assumes that packed data is a good idea.
 * Does the fairly trivial realloc thing for extension,
 * so .... ???
 * And you can keep adding cool things to it
 */

#pragma once

#include "../cf_types.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct cf_average_s {
	int			flags;
	uint32_t	n_points;
	uint64_t	points_sum;
} cf_average;

cf_average * cf_average_create( uint32_t initial_size, uint flags);
void cf_average_destroy( cf_average *avg);
void cf_average_clear(cf_average *avg);
int cf_average_add(cf_average *avgp, uint64_t value);   // warning! this fails if too many samples
double cf_average_calculate(cf_average *avg, bool clear);
