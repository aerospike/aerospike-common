/*
 *  Citrusleaf Foundation
 *  arenav.c - Arena allocator with variable-sized objects
 *
 *  Copyright 2012 by Aerospike.  All rights reserved.
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

// #define USE_MALLOC 1 // this helps debugging using valgrind 

static int cf_arenav_stage_add(cf_arenav *arena);

cf_arenav *
cf_arenav_create(uint32_t stage_bytes, uint max_stages, int flags)
{
	if (max_stages == 0) max_stages = 0xff;
	if (max_stages > 0xff)   {
		cf_warning(CF_ARENA, "could not create arena: too many stages: max stages %d out of bounds",max_stages);
		return(0);
	}
	
	cf_arenav *arena = cf_malloc(sizeof(cf_arenav) + (sizeof(void *) * max_stages));
	if (!arena)	{
		cf_warning(CF_ARENA, "malloc fail sz %"PRIu64,(sizeof(cf_arenav)+ (sizeof(void *) * max_stages)));
		return(0);
	}
	
	arena->flags = flags;
	
	if (flags | CF_ARENAV_MT_BIGLOCK)
		pthread_mutex_init(&arena->LOCK, 0);
	
	arena->stage_bytes = stage_bytes;

	arena->free = 0;
	
	arena->free_stage_id = 0;
	arena->free_element_ptr = 1; // burn the first so 0:0 is never in use (null)
	arena->stages[0] = cf_malloc(arena->stage_bytes);
	
	arena->max_stages = max_stages;
	for (uint i=1;i<max_stages;i++) {
		arena->stages[i] = 0;
	}

	return(arena);
}

void
cf_arenav_destroy(cf_arenav *arena)
{
	for (uint i=0;i<arena->max_stages;i++) {
		if (arena->stages[i] == 0) break;
		if (arena->flags & CF_ARENAV_EXTRACHECKS) memset(arena->stages[i], 0xff, arena->stage_bytes);
		cf_free(arena->stages[i]);
	}
	if (arena->flags & CF_ARENAV_EXTRACHECKS) memset(arena, 0xff, sizeof(cf_arenav));
	cf_free(arena);
}

static int
cf_arenav_stage_add(cf_arenav *arena)
{
	uint8_t *stage = cf_malloc(arena->stage_bytes);
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

	return(0);
}

#ifndef USE_MALLOC
void * 
cf_arenav_alloc(cf_arenav *arena, uint32_t element_sz)
{
	void *h;

	if (arena->flags & CF_ARENAV_MT_BIGLOCK)
		pthread_mutex_lock(&arena->LOCK);

	// look on the free list
	if (arena->free != 0) {
		h = arena->free;
		arena->free = arena->free->next;
	}
	// or slice out next element
	else {
		if (arena->free_element_ptr >= arena->stage_bytes) {
			cf_arenav_stage_add(arena);
			arena->free_stage_id++;
			arena->free_element_ptr = 0;
		}
		h = arena->stages[arena->free_stage_id] + (arena->free_element_ptr + element_sz);
		uint32_t *p = (uint32_t *) h;
		*p = arena->free_element_ptr;
		h += sizeof(uint32_t);
		arena->free_element_ptr += element_sz;
	}

	if (arena->flags & CF_ARENAV_MT_BIGLOCK)
		pthread_mutex_unlock(&arena->LOCK);

	if (arena->flags & CF_ARENAV_CALLOC) {
		memset(h, 0, element_sz);
	}

	return(h);
}

void cf_arenav_free(cf_arenav *arena, void *p)
{
	cf_arenav_free_element *e = p;

	// figure out what I'm inserting
	if (arena->flags & CF_ARENAV_EXTRACHECKS) {
		uint32_t element_sz = * (uint32_t *) (p - sizeof(uint32_t));
		memset(e, 0xff, element_sz);
		if (e->free_magic == CF_ARENAV_FREE_MAGIC) {
			cf_info(CF_ARENA, "warning: likely duplicate free of %x",p);
		}
	}

	// insert onto the free list
	if (arena->flags & CF_ARENAV_MT_BIGLOCK)
		pthread_mutex_lock(&arena->LOCK);

	e->next = arena->free;
	arena->free = e;

	if (arena->flags & CF_ARENAV_MT_BIGLOCK)
		pthread_mutex_unlock(&arena->LOCK);
}
#endif

#ifdef USE_MALLOC
void * 
cf_arenav_alloc(cf_arenav *arena, uint32_t element_sz) 
{
	return(cf_malloc(element_sz));
	
}

void 
cf_arenav_free(cf_arenav *arena, void *p)
{
	cf_free(p);
}
#endif // USE_MALLOC


/*
** cf_arenav_nuke(cf_arenav *arena):
**  Reset the arena to a blank state.
*/
void
cf_arenav_nuke(cf_arenav *arena)
{
	// XXX -- ***PSI***  9-Oct-2012
}

/*
****************************
**  ArenaV Unit Test Code
****************************
*/

typedef struct {
	int test1;
	int test2;
	int test3;
	uint8_t buf[100];
} arena_test_struct;

#define TEST1_MAGIC 0xFFEEDDCC
#define TEST2_MAGIC 0x99123456
#define TEST3_MAGIC 0x19191919

static
void arena_buf_set(uint8_t *buf, int len) {
	for (int i=0;i<len;i++)	buf[i] = i;
}

static
int arena_buf_validate(uint8_t *buf, int len) {
	for (int i=0;i<len;i++) if (buf[i] != i) return(-1);
	return(0);
}

int
cf_arenav_test()
{
	cf_info(CF_ARENA, " arena test ");
	
	uint32_t element_sz = 100; // XXX -- Use all 100 byte objects (not general.)
	int n_handles = (0xFFFF * 20) - 1;
	cf_arenav *arena = cf_arenav_create(sizeof(arena_test_struct) * 200, 0xFFFF, CF_ARENAV_EXTRACHECKS | CF_ARENAV_MT_BIGLOCK);
	if (!arena) {
		cf_info(CF_ARENA, "could not create first cf_arenav");
		return(-1);
	}
	
	void **objects = cf_malloc(sizeof(void *) * n_handles); 
//	int		*random_magic = cf_malloc(sizeof(int) * n_handles);
	
	// create as many objects are you are can
	for (int i =0; i< n_handles; i++) {
		objects[i] = cf_arenav_alloc(arena, element_sz);
		if (objects[i] == 0) { cf_info(CF_ARENA, "could not create some object: %d",i); continue; }
//		cf_info(CF_ARENA, "allocated object %x index %d",objects[i],i);
		arena_test_struct *ts = objects[i];
		if (ts == 0) { cf_info(CF_ARENA, "could not resolve some object: %x %d",objects[i], i); continue; }
//		cf_info(CF_ARENA, "arena resolve: pointer is %p",ts);
		memset(ts, 0, sizeof(*ts));
		ts->test1 = TEST1_MAGIC;
		ts->test2 = TEST2_MAGIC;
		ts->test3 = TEST3_MAGIC;
		arena_buf_set(ts->buf, sizeof(ts->buf));
		
	}
	
	for (int i=0;i< n_handles; i++) {
		arena_test_struct *ts = objects[i];
		if (ts->test1 != TEST1_MAGIC)	return(-1);
		if (ts->test2 != TEST2_MAGIC)	return(-1);
		if (ts->test3 != TEST3_MAGIC)	return(-1);
		if (0 != arena_buf_validate(ts->buf, sizeof(ts->buf)));
	}
	
	// free at least a few things
	for (uint i=0;i<100;i++) {
		cf_arenav_free(arena, objects[i*2]);
		objects[i*2] = 0;
	}
	
	// reallocate whatever was freed
	for (int i =0; i< n_handles; i++) {
		if (objects[i] == 0) {
			objects[i] = cf_arenav_alloc(arena, element_sz);
			if (objects[i] == 0) { cf_info(CF_ARENA, "could not create some object: %d",i); continue; }
	//		cf_info(CF_ARENA, "allocated object %x index %d",objects[i],i);
			arena_test_struct *ts = objects[i];
			if (ts == 0) { cf_info(CF_ARENA, "could not resolve some object: %x %d",objects[i], i); continue; }
	//		cf_info(CF_ARENA, "arena resolve: pointer is %p",ts);
			memset(ts, 0, sizeof(*ts));
			ts->test1 = TEST1_MAGIC;
			ts->test2 = TEST2_MAGIC;
			ts->test3 = TEST3_MAGIC;
			arena_buf_set(ts->buf, sizeof(ts->buf));
		}
	}

	// validate all
	for (int i=0;i< n_handles; i++) {
		arena_test_struct *ts = objects[i];
		if (ts->test1 != TEST1_MAGIC)	return(-1);
		if (ts->test2 != TEST2_MAGIC)	return(-1);
		if (ts->test3 != TEST3_MAGIC)	return(-1);
		if (0 != arena_buf_validate(ts->buf, sizeof(ts->buf)));
	}
	
	// qsort - let's make sure no one got the same thing
	
	// free everything again
	for (int i =0; i< n_handles; i++) {
		cf_arenav_free(arena, objects[i]);
		objects[i] = 0;
	}

	// allocate everything again
	for (int i =0; i< n_handles; i++) {
		if (objects[i] == 0) {
			objects[i] = cf_arenav_alloc(arena, element_sz);
			if (objects[i] == 0) { cf_info(CF_ARENA, "could not create some object: %d",i); continue; }
	//		cf_info(CF_ARENA, "allocated object %x index %d",objects[i],i);
			arena_test_struct *ts = objects[i];
			if (ts == 0) { cf_info(CF_ARENA, "could not resolve some object: %x %d",objects[i], i); continue; }
	//		cf_info(CF_ARENA, "arena resolve: pointer is %p",ts);
			memset(ts, 0, sizeof(*ts));
			ts->test1 = TEST1_MAGIC;
			ts->test2 = TEST2_MAGIC;
			ts->test3 = TEST3_MAGIC;
			arena_buf_set(ts->buf, sizeof(ts->buf));
		}
	}

	// validate all - one last time
	for (int i=0;i< n_handles; i++) {
		arena_test_struct *ts = objects[i];
		if (ts->test1 != TEST1_MAGIC)	return(-1);
		if (ts->test2 != TEST2_MAGIC)	return(-1);
		if (ts->test3 != TEST3_MAGIC)	return(-1);
		if (0 != arena_buf_validate(ts->buf, sizeof(ts->buf)));
	}
	
	cf_info(CF_ARENA, "**** arena test success!!!! *****\n");
	
	return(0);
}



