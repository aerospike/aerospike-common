/*
 *  Citrusleaf Foundation
 *  include/meminfo.h - Gathering information about the current memory use
 *
 *  Copyright 2010 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <openssl/ripemd.h>
#include <openssl/md4.h>
#include <stdio.h>


/* SYNOPSIS
 * We have the ability to evict data to protect the server. 
 */

// #define DEBUG_VERBOSE 1

int
cf_meminfo(uint64_t *physmem, uint64_t *freemem, bool *swapping);


