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

// round a value up to the nearest MODULUS

static inline uint32_t cf_roundup( uint32_t i, uint32_t modulus) {
	uint32_t t = i % modulus;
	if (t == 0)	return(i);
	return(  i + (modulus - t ) );
}

static inline uint64_t cf_roundup_64( uint64_t i, uint32_t modulus) {
	uint64_t t = i % modulus;
	if (t == 0)	return(i);
	return(  i + (modulus - t ) );
}

// some debate about what's faster

// #define cf_max_uint32(__x, __y) ( __x ^ (( __x ^ __y ) & -(__x < __y)) )
#define cf_max_uint32(__x, __y) ( (__x) > (__y) ? (__x) : (__y) )
// static inline uint32_t cf_max_uint32(uint32_t x, uint32_t y) {
//	return ( __x > __y ? __x : __y);
//}
#define cf_max(__x, __y) ( (__x) > (__y) ? (__x) : (__y) )

#ifdef MARCH_i686

static inline uint8_t * cf_roundup_p(uint8_t * p, uint32_t modulus) {
	uint32_t i = (uint32_t ) p;
	i = cf_roundup(i, modulus);
	return( (void *) i );
}
#elif MARCH_x86_64
static inline uint8_t * cf_roundup_p(uint8_t * p, uint32_t modulus) {
	uint64_t i = (uint64_t ) p;
	i = cf_roundup_64(i, modulus);
	return( (void *) i );
}
#else
    MISSING ARCHITECTURE
#endif


