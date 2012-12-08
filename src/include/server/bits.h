/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include "../cf_bits.h"
#include "../cf_types.h"
#include <strings.h>
#include <string.h>
#include <inttypes.h>
#include "arch.h"

/******************************************************************************
 * ASLIASES
 ******************************************************************************/

#define LogTable256 cf_LogTable256
#define bits_find_last_set cf_bits_find_last_set
#define bits_find_last_set_64 cf_bits_find_last_set_64
#define cf_bits_find_first_set(__x) ffs(__x)
#define cf_bits_find_first_set_64(__x) ffsll(__x)

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

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

