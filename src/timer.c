/*
 *  Citrusleaf Foundation
 *  src/timer.c - timer functionality
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"

#define DEBUG 1

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
	free(e);
}

//
// remember how condvars work.
// When you call condvar_wait, it releases the held mutex (must be called with mutex held)
// it unlocks mutex, waits until timeout or condvar signal or forever, 
//    reaquires mutex, and returns
//
int
cf_timer_worker_reduce_fn(cf_ll_element *ll, void *udata)
{
	cf_timer_element *e = (cf_timer_element *)ll;
	uint64_t	*now = (uint64_t *)udata;

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
	pthread_mutex_lock(&LOCK);
	while (1)
	{
		uint64_t now = cf_getms();
		cf_timer_expire_ms = 0;
		cf_ll_reduce(&cf_timer_list, true /*forward*/, cf_timer_worker_reduce_fn, &now); 
		
		if (cf_timer_expire_ms == 0) {
			cf_timer_expire_ms = 0;
			pthread_cond_wait(&CV, &LOCK);
		}
		// there's a new destination time, use it
		else {
			uint64_t ms = cf_timer_expire_ms - now;
			struct timespec tm;
			tm.tv_sec = ms / 1000; 
			tm.tv_nsec = (ms % 1000) * 1000;
			pthread_cond_timedwait(&CV, &LOCK, &tm);
		}
			
		// loop back around and wait!
	}
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
	cf_timer_element *e = (cf_timer_element *) malloc(sizeof(cf_timer_element));
	if (e == 0)	return(0);
	
	e->cb = cb;
	e->udata = udata;
	e->expire_ms = cf_getms() + ms;
	
	pthread_mutex_lock(&LOCK);
	
	cf_ll_insert_reduce(&cf_timer_list, (cf_ll_element *) e, 
		false /*fromend*/, cf_timer_add_reduce_fn, (void *) e); 
	
	// If the new element is shorter than the held interval,
	// signal the condvar so the worker can recompute
	if (e->expire_ms < cf_timer_expire_ms)
		pthread_cond_signal(&CV);
	
	pthread_mutex_lock(&LOCK);
	
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

