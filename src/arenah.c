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

int cf_arenah_stage_add(cf_arenah *arena);


cf_arenah *cf_arenah_create(uint element_sz, uint stage_sz, uint max_stages, int flags  )
{
	if (max_stages == 0) max_stages = 0xff;
	if (max_stages > 0xff)   {
		cf_warning(CF_ARENAH, "could not create arenah: stage size %d out of bounds",stage_sz);
		return(0);
	}
	
	if (stage_sz == 0) stage_sz = 0xFFFFFF;
	if (stage_sz > 0xFFFFFF) {
		cf_warning(CF_ARENAH, "coul dnot create arenah: stage size too large: %d",stage_sz);
		return(0);
	}
	
	if ( ((uint64_t)element_sz) * ((uint64_t)stage_sz) > 0xFFFFFFFFL) {
		cf_warning(CF_ARENAH, "could not create arenah, bytes per stage too large %"PRIu64,(uint64_t) element_sz * stage_sz);
		return(0);
	}
	
	cf_arenah *arena = cf_malloc( sizeof(cf_arenah) + (sizeof(void *) * max_stages));
	if (!arena)	{
		cf_warning(CF_ARENAH, "malloc fail sz %"PRIu64,(sizeof(cf_arenah)+ (sizeof(void *) * max_stages)) );
		return(0);
	}
	
	arena->flags = flags;
	
	if (flags | CF_ARENAH_MT_BIGLOCK)
		pthread_mutex_init(&arena->LOCK, 0);
	
	arena->element_sz = element_sz;
	arena->stage_sz = stage_sz;
	arena->stage_bytes = arena->element_sz * arena->stage_sz;

	arena->free = 0;
	
	arena->free_stage_id = 0;
	arena->free_element_id = 1; // burn the first so 0:0 is never in use (null)
	arena->stages[0] = cf_malloc(arena->stage_bytes);
	if (0 == arena->stages[0]) {
		cf_warning(CF_ARENAH, " could not create initial arena stage with %d bytes",arena->stage_bytes);
		cf_free(arena);
		return(0);
	}
	
	arena->max_stages = max_stages;
	for (uint i=1;i<max_stages;i++) {
		arena->stages[i] = 0;
	}

	return(arena);
}



void cf_arenah_destroy(cf_arenah *arena)
{
	for (uint i=0;i<arena->max_stages;i++) {
		if (arena->stages[i] == 0) break;
		if (arena->flags & CF_ARENAH_EXTRACHECKS) memset(arena->stages[i], 0xff, arena->stage_bytes);
		cf_free(arena->stages[i]);
	}
	if (arena->flags & CF_ARENAH_EXTRACHECKS) memset(arena, 0xff, sizeof(cf_arenah));
	cf_free(arena);
}

int cf_arenah_stage_add(cf_arenah *arena)
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


cf_arenah_handle 
cf_arenah_alloc(cf_arenah *arena )
{
	cf_arenah_handle h;
	
	if (arena->flags & CF_ARENAH_MT_BIGLOCK)
		pthread_mutex_lock(&arena->LOCK);
	
	// look on the free list
	if (arena->free != 0 ) {
		h = arena->free;
		cf_arenah_free_element * fe = cf_arenah_resolve(arena, h);
		arena->free = fe->next;
		
//		cf_arenah_handle_s *h_p = (cf_arenah_handle_s *) &h;
//		cf_info(CF_ARENAH, "alloc: arena %p fromfree: stageid %d elementid %d",arena,h_p->stage_id,h_p->element_id);
	}
	// or slice out next element
	else {
		if (arena->free_element_id >= arena->stage_sz) {
			cf_arenah_stage_add(arena);
			arena->free_stage_id++;
			arena->free_element_id = 0;
		}
		cf_arenah_handle_s hs;
		hs.stage_id = arena->free_stage_id;
		hs.element_id = arena->free_element_id;
		arena->free_element_id++;
		h = *(cf_arenah_handle *) &hs;

//		cf_arenah_handle_s *h_p = (cf_arenah_handle_s *) &h;
//		cf_info(CF_ARENAH, "alloc: arena %p new: stageid %d elementid %d",arena,h_p->stage_id,h_p->element_id);
	}
	
	if (arena->flags & CF_ARENAH_MT_BIGLOCK)
		pthread_mutex_unlock(&arena->LOCK);
	
	if (arena->flags & CF_ARENAH_CALLOC) {
		uint8_t *p = cf_arenah_resolve(arena, h);
		memset(p, 0, arena->element_sz);
	}
	
	return(h);
}


void cf_arenah_free(cf_arenah *arena, cf_arenah_handle h)
{
//	cf_arenah_handle_s *h_p = (cf_arenah_handle_s *) &h;
//	cf_info(CF_ARENAH, "free: arena %p new: stageid %d elementid %d",arena,h_p->stage_id,h_p->element_id);
	
	// figure out what I'm inserting
	cf_arenah_free_element *e = cf_arenah_resolve(arena, h);
	if (arena->flags & CF_ARENAH_EXTRACHECKS) {
		memset(e, 0xff, arena->element_sz);
		if (e->free_magic == CF_ARENAH_FREE_MAGIC) {
			cf_info(CF_ARENAH, "warning: likely duplicate free of handle %x",h);
		}
	}
	
	// insert onto the free list
	if (arena->flags & CF_ARENAH_MT_BIGLOCK)
		pthread_mutex_lock(&arena->LOCK);
	
	e->next = arena->free;
	arena->free = h;

	if (arena->flags & CF_ARENAH_MT_BIGLOCK)
		pthread_mutex_unlock(&arena->LOCK);
}

//
//  this is expensive - but sometimes worth it?
//    almost don't want to write this routine, that encourages people to use it
cf_arenah_handle
cf_arenah_pointer_resolve(cf_arenah *arena, void *ptr)
{
	uint8_t *pb = (uint8_t *)ptr;
	// never need to take lock because structure never gets rwitten
	for (int i=0 ; (arena->stages[i]) && (i<arena->max_stages) ; i++) {
		
		if (pb >= arena->stages[i]) {
			if (pb <= (arena->stages[i] + arena->stage_sz)) {
				cf_arenah_handle_s hs;
				hs.stage_id = i;
				hs.element_id = (pb - arena->stages[i]) / arena->element_sz;
				return( TO_HANDLE( hs ) );
			}
		}
	}
	
	return(0);
}

/*
****************************
**
**
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


void arenah_buf_set(uint8_t *buf, int len) {
	for (int i=0;i<len;i++)	buf[i] = i;
}


int arenah_buf_validate(uint8_t *buf, int len) {
	for (int i=0;i<len;i++) if (buf[i] != i) return(-1);
	return(0);
}

int
cf_arenah_test()
{
	cf_info(CF_ARENAH, " arena test ");
	
	int n_handles = (0xFFFF * 20) - 1;
	cf_arenah *arena = cf_arenah_create(sizeof(arena_test_struct), 0xFFFF, 200, CF_ARENAH_EXTRACHECKS | CF_ARENAH_MT_BIGLOCK);
	if (!arena ) {
		cf_info(CF_ARENAH, "could not create first arena");
		return(-1);
	}
	
	cf_arenah_handle *objects = cf_malloc( sizeof(cf_arenah_handle) * n_handles); 
//	int		*random_magic = cf_malloc( sizeof(int) * n_handles );
	
	// create as many objects are you are can
	for (int i =0; i< n_handles; i++) {
		objects[i] = cf_arenah_alloc(arena);
		if (objects[i] == 0) { cf_info(CF_ARENAH, "could not create some object: %d",i); continue; }
//		cf_info(CF_ARENAH, "allocated object %x index %d",objects[i],i);
		arena_test_struct *ts = cf_arenah_resolve(arena, objects[i]);
		if (ts == 0) { cf_info(CF_ARENAH, "could not resolve some object: %x %d",objects[i], i); continue; }
//		cf_info(CF_ARENAH, "arena resolve: pointer is %p",ts);
		memset(ts, 0, sizeof(*ts));
		ts->test1 = TEST1_MAGIC;
		ts->test2 = TEST2_MAGIC;
		ts->test3 = TEST3_MAGIC;
		arenah_buf_set(ts->buf, sizeof(ts->buf) );
		
	}
	
	for (int i=0;i< n_handles; i++) {
		arena_test_struct *ts = cf_arenah_resolve(arena, objects[i]);
		if (ts->test1 != TEST1_MAGIC)	return(-1);
		if (ts->test2 != TEST2_MAGIC)	return(-1);
		if (ts->test3 != TEST3_MAGIC)	return(-1);
		if (0 != arenah_buf_validate(ts->buf, sizeof(ts->buf)));
	}
	
	// free at least a few things
	for (uint i=0;i<100;i++) {
		cf_arenah_free(arena, objects[i*2]);
		objects[i*2] = 0;
	}
	
	// reallocate whatever was freed
	for (int i =0; i< n_handles; i++) {
		if (objects[i] == 0) {
			objects[i] = cf_arenah_alloc(arena);
			if (objects[i] == 0) { cf_info(CF_ARENAH, "could not create some object: %d",i); continue; }
	//		cf_info(CF_ARENAH, "allocated object %x index %d",objects[i],i);
			arena_test_struct *ts = cf_arenah_resolve(arena, objects[i]);
			if (ts == 0) { cf_info(CF_ARENAH, "could not resolve some object: %x %d",objects[i], i); continue; }
	//		cf_info(CF_ARENAH, "arena resolve: pointer is %p",ts);
			memset(ts, 0, sizeof(*ts));
			ts->test1 = TEST1_MAGIC;
			ts->test2 = TEST2_MAGIC;
			ts->test3 = TEST3_MAGIC;
			arenah_buf_set(ts->buf, sizeof(ts->buf) );
		}
	}

	// validate all
	for (int i=0;i< n_handles; i++) {
		arena_test_struct *ts = cf_arenah_resolve(arena, objects[i]);
		if (ts->test1 != TEST1_MAGIC)	return(-1);
		if (ts->test2 != TEST2_MAGIC)	return(-1);
		if (ts->test3 != TEST3_MAGIC)	return(-1);
		if (0 != arenah_buf_validate(ts->buf, sizeof(ts->buf)));
	}
	
	// qsort - let's make sure no one got the same thing
	
	// free everything again
	for (int i =0; i< n_handles; i++) {
		cf_arenah_free(arena, objects[i]);
		objects[i] = 0;
	}

	// allocate everything again
	for (int i =0; i< n_handles; i++) {
		if (objects[i] == 0) {
			objects[i] = cf_arenah_alloc(arena);
			if (objects[i] == 0) { cf_info(CF_ARENAH, "could not create some object: %d",i); continue; }
	//		cf_info(CF_ARENAH, "allocated object %x index %d",objects[i],i);
			arena_test_struct *ts = cf_arenah_resolve(arena, objects[i]);
			if (ts == 0) { cf_info(CF_ARENAH, "could not resolve some object: %x %d",objects[i], i); continue; }
	//		cf_info(CF_ARENAH, "arena resolve: pointer is %p",ts);
			memset(ts, 0, sizeof(*ts));
			ts->test1 = TEST1_MAGIC;
			ts->test2 = TEST2_MAGIC;
			ts->test3 = TEST3_MAGIC;
			arenah_buf_set(ts->buf, sizeof(ts->buf) );
		}
	}

	// validate all - one last time
	for (int i=0;i< n_handles; i++) {
		arena_test_struct *ts = cf_arenah_resolve(arena, objects[i]);
		if (ts->test1 != TEST1_MAGIC)	return(-1);
		if (ts->test2 != TEST2_MAGIC)	return(-1);
		if (ts->test3 != TEST3_MAGIC)	return(-1);
		if (0 != arenah_buf_validate(ts->buf, sizeof(ts->buf)));
	}
	
	cf_info( CF_ARENAH, "**** arena test success!!!! *****\n");
	
	return(0);
}



