/*
 *  Citrusleaf Foundation
 *  include/arenav.h - Arena allocator with variable-sized objects
 *
 *  Copyright 2012 by Aerospike.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include "atomic.h"


/* SYNOPSIS
 * Arena allocation
 * When a single structure is very common, and of static size,
 * a great deal of speed and space can be gained by using
 * a straightforward arena system. There are two general kinds of arena
 * allocators: one which simply runs another general purpose allocator
 * internally, generally good for reducing free time & providing sandboxing,
 * and large-scale arenas which tend not to get destroyed. This is the second
 * type.
 */
 
 //
 // Note about arena handles. We want 0 to be 'null', so we burn 0:0
 // and just don't use it. 1:0 is legal. 


#define CF_ARENAV_FREE_MAGIC 0xabcd1234

typedef struct cf_arenav_free_element_s {
	struct cf_arenav_free_element_s *next;
	unsigned int free_magic;
} cf_arenav_free_element;


typedef struct cf_arenav_s {
	
	int flags;
	
	// a single mutex might create hot spots. Let's see.
	pthread_mutex_t	LOCK;

	// a free list makes many allocations quick
	cf_arenav_free_element *free;

	uint32_t free_stage_id;    // stage currently end-allocating from
	uint32_t free_element_ptr; // element currently end-allocating from
	
	// constants
	uint32_t stage_bytes;  // number of bytes in a stage (saves multiplication sometimes)
	
	uint32_t max_stages;
	uint8_t *stages[];
	
} cf_arenav;

#define CF_ARENAV_MT_BIGLOCK 	(1 << 0)
#define CF_ARENAV_EXTRACHECKS 	(1 << 1)
#define CF_ARENAV_CALLOC 		(1 << 2)


extern cf_arenav *cf_arenav_create(uint32_t stage_bytes, uint max_stages , int flags);
extern void cf_arenav_destroy(cf_arenav *arena);

extern void *cf_arenav_alloc(cf_arenav *arena, uint32_t element_sz);
extern void cf_arenav_free(cf_arenav *arena, void *ptr);

extern void cf_arenav_nuke(cf_arenav *arena);


/* an external unit test */
extern int cf_arenav_test();

