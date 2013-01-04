/*
 *      cf_average.h
 *
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
 *
 */

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
