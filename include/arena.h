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

// the struct we use internally to resolve
typedef struct cf_arena_handle_s {
	uint32_t	stage_id:8;
 	uint32_t	element_id:24;
} __attribute__ ((__packed__)) cf_arena_handle_s;

// external people use this
typedef uint32_t cf_arena_handle;

#define CF_FREE_MAGIC 0x9ff71239

typedef struct cf_arena_free_element_s {
	cf_arena_handle next;
	unsigned int    free_magic;
} cf_arena_free_element;

// big cast
#define TO_HANDLE( __hs ) ( *(uint32_t *) &__hs )

typedef struct cf_arena_s {
	
	int flags;
	
	// a single mutex might create hot spots. Let's see.
	pthread_mutex_t	LOCK;

	// a free list makes many allocations quick
	cf_arena_handle    free;

	uint32_t     free_stage_id;   // stage currently end-allocating from
	uint32_t     free_element_id; // element currently end-allocating from
	
	// constants
	uint32_t element_sz;   // number of bytes in an element, say, 54
	uint32_t stage_sz;     // number of elements in a stage; 
	uint32_t stage_bytes;  // number of bytes in a stage (saves multiplication sometimes)
	
	
	uint32_t	max_stages;
	uint8_t    *stages[];
	
} cf_arena;

#define CF_ARENA_MT_BIGLOCK 	(1 << 0)
#define CF_ARENA_EXTRACHECKS 	(1 << 1)
#define CF_ARENA_CALLOC 		(1 << 2)


extern cf_arena *cf_arena_create(uint element_sz, uint stage_sz, uint max_stages , int flags);
extern void cf_arena_destroy(cf_arena *arena);

extern cf_arena_handle cf_arena_alloc(cf_arena *arena);
extern void cf_arena_free(cf_arena *arena, cf_arena_handle h);


static inline
void *cf_arena_resolve(cf_arena *arena, cf_arena_handle h)
{
	cf_arena_handle_s *h_p = (cf_arena_handle_s *) &h;
//	fprintf(stderr, "arena %p elem_sz %d stage_id %d element_id %d\n",arena,arena->element_sz,h_p->stage_id, h_p->element_id);
	return( arena->stages[h_p->stage_id] + (h_p->element_id * arena->element_sz) );
}

extern cf_arena_handle cf_arena_pointer_resolve(cf_arena *arena, void *ptr);

/* an external unit test */
extern int cf_arena_test();

