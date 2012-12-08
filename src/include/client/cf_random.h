/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include <inttypes.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

extern int cf_get_rand_buf(uint8_t *buf, int len);
extern uint64_t cf_get_rand64();
extern uint32_t cf_get_rand32();