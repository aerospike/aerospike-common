/*
 *  Citrusleaf Foundation
 *  include/arena.c - arena allocator
 *
 *  Copyright 2010 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "cf.h"


// #define EXTRA_CHECKS 1

int cf_arena_stage_add(cf_arena *arena);


cf_arena *cf_arena_create(uint element_sz, uint stage_sz, uint max_stages )
{
	cf_arena *arena = malloc( sizeof(cf_arena) + (sizeof(cf_arena_stage) * max_stages));
	if (!arena)	return(0);
	
	if (stage_sz > 0xFFFFFF) return(0);
	
	pthread_mutex_init(&arena->LOCK, 0);
	arena->element_sz = element_sz;
	arena->stage_sz = stage_sz;

	CF_ARENA_HANDLE_NULL( arena->free );
	
	arena->free_stage_id = 0;
	arena->free_element_id = 0;
	
	arena->max_stages = max_stages;
	for (uint i=0;i<max_stages;i++) {
		arena->stages[i] = 0;
	}
	
	cf_arena_stage_add(arena);

	return(arena);
}



void cf_arena_destroy(cf_arena *arena)
{
	for (uint i=0;i<arena->max_stages;i++) {
		if (arena->stages[i] == 0) break;
		free(arena->stages[i]);
	}
		
	free(arena);
}

int cf_arena_stage_add(cf_arena *arena)
{
	cf_arena_stage *stage = malloc(arena->element_sz * arena->stage_sz);
	if (!stage) {
		return(-1);
	}
	uint i;
	for (i=0;i<arena->max_stages;i++) {
		if (arena->stages[i] == 0) {
			arena->stages[i] = stage;
			break;
		}
	}

	arena->free_stage_id = i;
	arena->free_element_id = 0;
	return(0);
}


cf_arena_handle 
cf_arena_alloc(cf_arena *arena )
{
	cf_arena_handle h;
	
	pthread_mutex_lock(&arena->LOCK);
	
	// look on the free list
	if (! CF_ARENA_HANDLE_ISNULL(arena->free) ) {
		h = arena->free;
		cf_arena_free_element *e = arena_handle_resolve(arena, h);
		arena->free = e->next;
	}
	// look in the next arena
	else {
		h.stage_id = arena->free_stage_id;
		h.element_id = arena->free_element_id;
		arena->free_element_id++;
		if (arena->free_element_id >= arena->stage_sz) {
			cf_arena_stage_add(arena);
			arena->free_stage_id++;
			arena->free_element_id = 0;
		}
	}
	
	pthread_mutex_unlock(&arena->LOCK);
	return(h);
}


void cf_arena_free(cf_arena *arena, cf_arena_handle h)
{
	pthread_mutex_lock(&arena->LOCK);
	
	// insert onto the free list
	cf_arena_free_element *e = arena_handle_resolve(arena, h);
	e->next = arena->free;
	arena->free = h;
	
	pthread_mutex_unlock(&arena->LOCK);
	
}



