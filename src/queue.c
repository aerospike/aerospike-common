/*
 *  Citrusleaf Foundation
 *  src/queue.c - queue framework
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



/* SYNOPSIS
 * Queue
 */


/* cf_queue_create
 * Initialize a queue */
cf_queue *
cf_queue_create(size_t elementsz)
{
	cf_queue *q = NULL;

	q = malloc( sizeof(cf_queue));
	/* FIXME error msg */
	if (!q)
		return(NULL);
	q->allocsz = CF_QUEUE_ALLOCSZ;
	q->utilsz = 0;
	q->elementsz = elementsz;

	q->queue = malloc(CF_QUEUE_ALLOCSZ * elementsz);
	if (! q->queue) {
		free(q);
		return(NULL);
	}
	
	if (0 != pthread_mutex_init(&q->LOCK, NULL)) {
		/* FIXME error msg */
		free(q->queue);
		free(q);
		return(NULL);
	}

	if (0 != pthread_cond_init(&q->CV, NULL)) {
		pthread_mutex_destroy(&q->LOCK);
		free(q->queue);
		free(q);
		return(NULL);
	}

	return(q);
}

// TODO: probably need to wait until whoever is inserting or removing is finished 

void
cf_queue_destroy(cf_queue *q)
{
	pthread_cond_destroy(&q->CV);
	pthread_mutex_destroy(&q->LOCK);
	memset(q->queue, 0, sizeof(q->allocsz * q->elementsz));
	free(q->queue);
	memset(q, 0, sizeof(cf_queue) );
	free(q);
}

int
cf_queue_sz(cf_queue *q)
{
	int rv;
	
	pthread_mutex_lock(&q->LOCK);
	rv = q->utilsz;
	pthread_mutex_unlock(&q->LOCK);
	return(rv);
	
}


/* cf_queue_push
 * */
int
cf_queue_push(cf_queue *q, void *ptr)
{
	/* FIXME arg check */

	/* FIXME error */
	if (0 != pthread_mutex_lock(&q->LOCK))
		return(-1);

	/* Check queue length */
	if (q->allocsz == q->utilsz) {
		/* FIXME freak out */
		D("QQQQ: realloc!");
		q->allocsz += CF_QUEUE_ALLOCSZ;
		if (NULL == (q->queue = realloc(q->queue, q->allocsz * q->elementsz)))
			return(-1);
	}

	memcpy(&q->queue[q->utilsz * q->elementsz], ptr, q->elementsz);
	q->utilsz++;
	pthread_cond_signal(&q->CV);

	/* FIXME blow a gasket */
	if (0 != pthread_mutex_unlock(&q->LOCK))
		return(-1);

	return(0);
}


/* cf_queue_pop
 * if ms_wait < 0, wait forever
 * if ms_wait = 0, don't wait at all
 * if ms_wait > 0, wait that number of ms
 * */
int
cf_queue_pop(cf_queue *q, void *buf, int ms_wait)
{
	if (NULL == q)
		return(-1);

	/* FIXME error checking */
	if (0 != pthread_mutex_lock(&q->LOCK))
		return(-1);

	struct timespec tp;
	if (ms_wait > 0) {
		clock_gettime( CLOCK_REALTIME, &tp); 
		tp.tv_sec += ms_wait / 1000;
		tp.tv_nsec += ms_wait % 1000;
		if (tp.tv_nsec > 1000000000) {
			tp.tv_nsec -= 1000000000;
			tp.tv_sec++;
		}
	}

	/* FIXME error checking */
	/* Note that we apparently have to use a while() loop.  Careful reading
	 * of the pthread_cond_signal() documentation says that AT LEAST ONE
	 * waiting thread will be awakened... */
	while (0 == q->utilsz) {
		if (CF_QUEUE_FOREVER == ms_wait) {
			pthread_cond_wait(&q->CV, &q->LOCK);
		}
		else if (CF_QUEUE_NOWAIT == ms_wait) {
			pthread_mutex_unlock(&q->LOCK);
			return(CF_QUEUE_EMPTY);
		}
		else {
			pthread_cond_timedwait(&q->CV, &q->LOCK, &tp);
			if (0 == q->utilsz)
				return(CF_QUEUE_EMPTY);
		}
	}

	/* This is a naive FIFO queue; we ought to come up with a nicer
	 * implementation at some point */
	memcpy(buf, &q->queue[0], q->elementsz);
	q->utilsz--;
	memmove(&q->queue[0], &q->queue[0] + q->elementsz, q->elementsz * q->utilsz);

	/* FIXME blow a gasket */
	if (0 != pthread_mutex_unlock(&q->LOCK)) {
		fprintf(stderr, "unlock failed\n");
		return(-1);
	}

	return(0);
}

// Iterate over all queue members calling the callback

int 
cf_queue_reduce(cf_queue *q,  cf_queue_reduce_fn cb, void *udata)
{
	if (NULL == q)
		return(-1);

	/* FIXME error checking */
	if (0 != pthread_mutex_lock(&q->LOCK))
		return(-1);

	if (q->utilsz) {
		byte *qp = q->queue;
		byte *qbase = qp;
		// this is a little goofy, but we want to delete while
		// doing the callback, this is always up to date
		while (qp - qbase < q->utilsz * q->elementsz) {
			
			int rv = cb(qp, udata);
			
			if (rv == 0) {
				qp += q->elementsz;
			}
			else if (rv == -1) {
				break; // found what it was looking for
			}
			else if (rv == -2) { // delete!
				int curpos = (qp - qbase) / q->elementsz;
				if (curpos != q->utilsz - 1) {
					memmove(qp, qp+q->elementsz, (q->utilsz - curpos - 1) * q->elementsz);
				}	
				q->utilsz--;
			}
		};
	}
	
	if (0 != pthread_mutex_unlock(&q->LOCK)) {
		fprintf(stderr, "unlock failed\n");
		return(-1);
	}

	return(0);
	
}

// Special case: delete elements from the queue
// pass 'true' as the 'only_one' parameter if you know there can be only one element
// with this value on the queue

int
cf_queue_delete(cf_queue *q, void *buf, bool only_one)
{
	if (NULL == q)
		return(CF_QUEUE_ERR);

	/* FIXME error checking */
	if (0 != pthread_mutex_lock(&q->LOCK))
		return(CF_QUEUE_ERR);

	bool found = false;
	
	if (q->utilsz) {
		byte *qp = q->queue;
		byte *qbase = qp;
		// this is a little goofy, but we want to delete while
		// doing the callback, this is always up to date
		while (qp - qbase < q->utilsz * q->elementsz) {
			
			int rv = memcmp(qp, buf, q->elementsz);
			
			if (rv != 0) {
				qp += q->elementsz;
			}
			else if (rv == 0) { // delete!
				int curpos = (qp - qbase) / q->elementsz;
				if (curpos != q->utilsz - 1) {
					memmove(qp, qp+q->elementsz, (q->utilsz - curpos - 1) * q->elementsz);
				}	
				q->utilsz--;
				found = true;
				if (only_one == true)	goto Done;
			}
		};
	}
	
Done:	
	if (0 != pthread_mutex_unlock(&q->LOCK)) {
		fprintf(stderr, "unlock failed\n");
		return(-1);
	}

	if (found == false)
		return(CF_QUEUE_EMPTY);
	else
		return(CF_QUEUE_OK);
	
}
