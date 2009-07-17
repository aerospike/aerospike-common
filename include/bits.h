/*
 *  Citrusleaf Foundation
 *  include/hist.h - timer functionality
 *
 *  Copyright 2009 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once

#include <strings.h>
#include <string.h>
#include <inttypes.h>

/* SYNOPSIS
 * Some bithacks are eternal and handy
 * http://graphics.stanford.edu/~seander/bithacks.html
 */

#define bits_find_first_set(__x) ffs(__x)
#define bits_find_first_set_64(__x) ffsll(__x)
 
extern int bits_find_last_set(uint32_t c);
extern int bits_find_last_set_64(uint64_t c);

static const char LogTable256[] =
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
	LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

