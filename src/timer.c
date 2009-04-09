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
	struct cf_timer_element_s *next;
	struct cf_timer_element_s *prev;
	cf_timer_fn			cb;
	void 				*udata;
	uint64_t			expire_ms;
} cf_timer_element;

static pthread_mutex_t	LOCK;
static pthread_cond_t	CV;
static pthread_t  		cf_timer_worker;
static uint64_t			cf_timer_expire_ms;

static cf_timer_element *head = 0;
static cf_timer_element *tail = 0;

//
// remember how condvars work.
// When you call condvar_wait, it releases the held mutex (must be called with mutex held)
// it unlocks mutex, waits until timeout or condvar signal or forever, 
//    reaquires mutex, and returns
//


void *
cf_timer_worker_fn(void *gcc_is_ass)
{
	// forever
	pthread_mutex_lock(&LOCK);
	while (1)
	{
		uint64_t now;
		// nothin in queue, wait forever
		if (head == 0) {
			cf_timer_expire_ms = 0;
			pthread_cond_wait(&CV, &LOCK);
		}
		// somethin in queue, wait until then
		else {
			now = cf_getms();
			if (now < head->expire_ms) {
				uint64_t ms = head->expire_ms - now;
				cf_timer_expire_ms = head->expire_ms;
				struct timespec tm;
				tm.tv_sec = ms / 1000; 
				tm.tv_nsec = (ms % 1000) * 1000;
				pthread_cond_timedwait(&CV, &LOCK, &tm);
			}
		}
			
		// are there expired elements? fire them!
		now = cf_getms();
		cf_timer_element *cur = head;
		while (cur && (cur->expire_ms < now)) {
			(*cur->cb) (cur->udata);
			// delete this one - happen to know it's a head delete though
			head = cur->next;
			cur->next->prev = 0;
			// free this element
			cf_timer_element *_t = cur;
			cur = cur->next;
			free(_t);
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
	head = 0;
	tail = 0;
	
	pthread_create(&cf_timer_worker, 0, cf_timer_worker_fn, 0);
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
	
	// Thread the element on the list
	
	// First element
	if (head == 0)
	{
		head = e;
		tail = e;
		e->next = 0;
		e->prev = 0;
	}
	else
	{
		cf_timer_element *cur = tail;
		while (cur && (e->expire_ms < cur->expire_ms))
		{
			cur = cur->prev;
		}

		// head insert
		if (cur == 0) {
			e->next = head;
			head = e;
			e->next->prev = e;
			e->prev = 0;
		}
		else if (cur == tail) {
			// tail insert
			tail->next = e;
			e->prev = tail;
			tail = e;
			e->next = 0;
		}
		else { // we're in the middle somewhere - we want to be behind cur
			e->prev = cur;
			e->next = cur->next;
			cur->next->prev = e;
			cur->next = e;
		}
	}
	
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
	cf_timer_element *e = hand;
	
	pthread_mutex_lock(&LOCK);
	
	// unlink
	if (e == head) {
		if (e == tail) {
			head = 0;
			tail = 0;
		}
		else {
			head = e->next;
			e->next->prev = 0;
		}
	}
	else if (e == tail) {
		tail = e->prev;
		e->prev->next = 0;
	}
	else {
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
	free(e);
	
	pthread_mutex_unlock(&LOCK);
	
	return;
}

