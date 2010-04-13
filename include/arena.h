/*
 *  Citrusleaf Foundation
 *  include/arena.h - Arena allocator
 *
 *  Copyright 2010 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include <stdint.h>
#include <pthread.h>
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

typedef struct cf_arena_handle_s {
	uint32_t	stage_id:8;
 	uint32_t	element_id:24;
} __attribute__ ((__packed__)) cf_arena_handle;

typedef struct cf_arena_free_element_s {
	cf_arena_handle next;
} cf_arena_free_element;

typedef struct cf_arena_stage_s {
	uint32_t   free_object_id;    //
	
	uint8_t data[];
} cf_arena_stage;


typedef struct cf_arena_s {
	// a single mutex might create hot spots. Let's see.
	pthread_mutex_t	LOCK;
	
	// constants
	uint32_t element_sz;   // number of bytes in an element, say, 54
	uint32_t stage_sz;     // number of elements in a stage; 
	
	// a free list makes many allocations quick
	cf_arena_handle    free;

	uint32_t     free_stage_id;   // stage currently end-allocating from
	uint32_t     free_element_id; // element currently end-allocating from
	
	uint32_t	max_stages;
	cf_arena_stage *stages[];
	
} cf_arena;
 
extern cf_arena *cf_arena_create(uint element_sz, uint stage_sz, uint max_stages );
extern void cf_arena_destroy(cf_arena *arena);

extern cf_arena_handle cf_arena_alloc(cf_arena *arena);
extern void cf_arena_free(cf_arena *arena, cf_arena_handle h);

// free is an inline

// set a handle to null
#define CF_ARENA_HANDLE_NULL( __h ) (*(uint32_t *) &__h) = 0
#define CF_ARENA_HANDLE_ISNULL( __h ) ((*(uint32_t *) &__h) == 0)

static inline
void *arena_handle_resolve(cf_arena *arena, cf_arena_handle h)
{
	return( ((uint8_t *)arena->stages[h.stage_id]) + (h.element_id * arena->element_sz) );
}



