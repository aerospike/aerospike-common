/*
 *  Citrusleaf Foundation
 *  src/timer.c - timer functionality
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include "server/cf.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

// #define DEBUG 1

/* SYNOPSIS
 * Timer
 * A general purpose timer library, whereby you can register a 
 * function pointer to be called at a specific time
 * This kind of code is typically implemented as a RB-tree for fast inserts.
 * however, a bucketized-list system works very well too. I will implement
 * a single-bucket to start, as I know these timers are used for expirations,
 * and rarely fire. Instead, they're almost always cancelled before they fire.
 * Thus, make insert-far-in-future and cancel very efficient.
 *
 * Currently support only one-shot timers, nothing repeating
 * only allows cancel, not change-time (cancel and add again if you want that)
 */
 
/* cf_timer_element
 * An internal element queue */

typedef struct cf_timer_element_s {
	cf_ll_element	    ll_e;
	cf_timer_fn			cb;
	void 				*udata;
	uint64_t			expire_ms;
} cf_timer_element;

static pthread_mutex_t	LOCK;
static pthread_cond_t	CV;
static pthread_t  		cf_timer_worker;
static uint64_t			cf_timer_expire_ms;

static cf_ll cf_timer_list;

void
cf_timer_destructor_fn(cf_ll_element *e)
{
	cf_free(e);
}

//
// Debug stuff
//'

int
cf_timer_dump_fn(cf_ll_element *le, void *udata)
{
	cf_timer_element *e = (cf_timer_element *)le;
	cf_info(CF_TIMER, "dump: expire %"PRIu64" cb %p udata %p",e->expire_ms,e->cb,e->udata);
	return(0);
}

void
cf_timer_dump_all( )
{
	cf_info(CF_TIMER, "dumping timer list");
	cf_ll_reduce(&cf_timer_list, true, cf_timer_dump_fn, 0);
}


//
// remember how condvars work.
// When you call condvar_wait, it releases the held mutex (must be called with mutex held)
// it unlocks mutex, waits until timeout or condvar signal or forever, 
//    reaquires mutex, and returns
//
int
cf_timer_worker_reduce_fn(cf_ll_element *le, void *udata)
{
	cf_timer_element *e = (cf_timer_element *)le;
	uint64_t	*now = (uint64_t *)udata;

	cf_detail(CF_TIMER, "worker reduce: expire %"PRIu64" now %"PRIu64, e->expire_ms, *now);
	
	if (e->expire_ms <= *now) {
		(*e->cb) (e->udata);
		return(CF_LL_REDUCE_DELETE);
	}
	
	cf_timer_expire_ms = e->expire_ms;
	return(-1);
}

void *
cf_timer_worker_fn(void *gcc_is_ass)
{
	// forever
	cf_info(CF_TIMER, "worker start!");
	pthread_mutex_lock(&LOCK);
	while (1)
	{
		// DEBUG!
#ifdef DEBUG		
		cf_timer_dump_all();
#endif		

		uint64_t now = cf_getms();
		cf_timer_expire_ms = 0;
		cf_detail(CF_TIMER, "worker fn reducing: now %"PRIu64,now);
		cf_ll_reduce(&cf_timer_list, true /*forward*/, cf_timer_worker_reduce_fn, &now); 

		
		if (cf_timer_expire_ms == 0) {
			
			pthread_cond_wait(&CV, &LOCK);
		}
		// there's a new destination time, use it
		else {
			uint64_t ms = cf_timer_expire_ms - now;
			struct timespec tm;
			tm.tv_sec = ms / 1000; 
			tm.tv_nsec = (ms % 1000) * 1000000;
			pthread_cond_timedwait(&CV, &LOCK, &tm);
		}
			
		// loop back around and wait!
	}
	return(0);
}


int
cf_timer_init()
{

	if (0 != pthread_mutex_init(&LOCK, NULL)) {
		return(-1);
	}

	if (0 != pthread_cond_init(&CV, NULL)) {
		pthread_mutex_destroy(&LOCK);
		return(-1);
	}
	cf_timer_expire_ms = 0;
	cf_ll_init(&cf_timer_list, cf_timer_destructor_fn, false);
	
	pthread_create(&cf_timer_worker, 0, cf_timer_worker_fn, 0);
	return(0);
}

int
cf_timer_add_reduce_fn(cf_ll_element *ll, void *udata)
{
	if (ll == 0)	return(CF_LL_REDUCE_INSERT);
	
	cf_timer_element *cur = (cf_timer_element *) ll;
	cf_timer_element *e = (cf_timer_element *)udata;
	
	if (cur->expire_ms < e->expire_ms)
		return(CF_LL_REDUCE_INSERT);
	else
		return(0);
}


cf_timer_handle *
cf_timer_add(uint32_t ms, cf_timer_fn cb, void *udata)
{
	cf_timer_element *e = (cf_timer_element *) cf_malloc(sizeof(cf_timer_element));
	if (e == 0)	return(0);
	
	e->cb = cb;
	e->udata = udata;
	e->expire_ms = cf_getms() + ms;
	
	pthread_mutex_lock(&LOCK);
	
	cf_ll_insert_reduce(&cf_timer_list, (cf_ll_element *) e, 
		false /*fromend*/, cf_timer_add_reduce_fn, (void *) e); 
	
	// If the new element is shorter than the held interval,
	// signal the condvar so the worker can recompute
	if ((cf_timer_expire_ms == 0) || (e->expire_ms < cf_timer_expire_ms))
		pthread_cond_signal(&CV);
	
	pthread_mutex_unlock(&LOCK);
	
	return(e);
}

void
cf_timer_cancel(cf_timer_handle *hand)
{
	cf_ll_element *e = (cf_ll_element *)hand;
	
	pthread_mutex_lock(&LOCK);

	cf_ll_delete(&cf_timer_list, e);
		
	pthread_mutex_unlock(&LOCK);
	
	return;
}

//
// UNIT TESTS, at least some basic stuff
//

#define TEST_TIMER_MAX 50

int g_timer_test_info[TEST_TIMER_MAX];


int
cf_timer_test_fn(void *udata)
{
	int 	i = (int) (size_t) udata;
	cf_debug(CF_TIMER, "timer %d fired at %"PRIu64,i,cf_getms() );
	g_timer_test_info[i]++;
	return(0);
}

int
cf_timer_test()
{
	cf_info(CF_TIMER, "running timer test - start time %"PRIu64,cf_getms() );
	
	// FIRE TEST
	// insert N timer elements, watch them fire
	for (uint i=0;i<TEST_TIMER_MAX;i++) {
		g_timer_test_info[i] = (size_t) 0;
		// Heinous cast required to keep the compiler quiet
		cf_timer_add(10, cf_timer_test_fn, (void *)((size_t)i));
		usleep(1000);
	}
	cf_info(CF_TIMER, "last timer added: now %"PRIu64,cf_getms());
	
	usleep(1000 * 1000);
	
	cf_info(CF_TIMER, "check: timers fired? now %"PRIu64,cf_getms());
	for (uint i=0;i<TEST_TIMER_MAX;i++) {
		if (g_timer_test_info[i] != 1) {
			cf_debug(CF_TIMER, "timer %d did not fire in a timely fashion!",i);
			return(-1);
		}
	}
	cf_info(CF_TIMER, "yes, all have fired!");
	

	// CANCEL TEST
	// insert N timer elements, cancel them all, none fire
	cf_info(CF_TIMER, " **** setting up for cancel test ***** ");
	cf_timer_handle *timer_handles[TEST_TIMER_MAX];
	for (uint i=0;i<TEST_TIMER_MAX;i++) {
		// Heinous cast required to keep the compiler quiet
		timer_handles[i] = cf_timer_add(1000, cf_timer_test_fn, (void *)((size_t)i));
		usleep(1000);
	}
	for (uint i=0;i<TEST_TIMER_MAX;i++) {
		cf_timer_cancel(timer_handles[i]);
		usleep(1000);
	}
	usleep( 1000 * 1000 );
	for (uint i=0;i<TEST_TIMER_MAX;i++) {
		if (g_timer_test_info[i] != 1) {
			cf_debug(CF_TIMER, "timer %d must have fired, shouldn't have! (%d)",i,g_timer_test_info[i]);
			return(-1);
		}
	}
	cf_info(CF_TIMER, "*** cancel test passed");
	
	return(0);
	
}

