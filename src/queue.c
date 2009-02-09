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

#define DEBUG 1

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
	q->write_offset = q->read_offset = 0;
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
// Sort of. Anyone in a race with the destructor, who has a pointer to the queue,
// is in jepardy anyway.

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
	rv = CF_Q_SZ(q);
	pthread_mutex_unlock(&q->LOCK);
	return(rv);
	
}

//
// Internal function. Call with new size with lock held.
// *** THIS ONLY WORKS ON FULL QUEUES ***
//
static int
cf_queue_resize(cf_queue *q, uint new_sz)
{
	// check - a lot of the code explodes badly if queue is not full
	if (CF_Q_SZ(q) != q->allocsz) {
		D("cf_queue: internal error: resize on non-full queue");
		return(-1);
	}
	
	// the rare case where the queue is not fragmented, and realloc makes sense
	// and none of the offsets need to move
	if (0 == q->read_offset % q->allocsz) {
		q->queue = realloc(q->queue, new_sz * q->elementsz);
		if (!q->queue) {
			D(" pfft! out of memory! crash!");
			return(-1);
		}
		q->read_offset = 0;
		q->write_offset = q->allocsz;
	}
	else {
		
		byte *newq = malloc(new_sz * q->elementsz);
		if (!newq) {
			D(" pffth! out of memory! crash!");
			return(-1);
		}
		// endsz is used bytes in the old queue from the insert point to the end
		uint endsz = (q->allocsz - (q->read_offset % q->allocsz)) * q->elementsz;
		memcpy(&newq[0], CF_Q_ELEM_PTR(q, q->read_offset), endsz);
		memcpy(&newq[endsz], &q->queue[0], (q->allocsz * q->elementsz) - endsz); 
		
		free(q->queue);
		q->queue = newq;

		q->write_offset = q->allocsz;
		q->read_offset = 0;
	}

	q->allocsz = new_sz;
	return(0);	
}

// we have to guard against wraparound, call this occasionally
// I really expect this will never get called....
static void
cf_queue_unwrap(cf_queue *q)
{
	q->read_offset %= q->allocsz;
	q->write_offset %= q->allocsz;
}


/* cf_queue_push
 * Push goes to the front, which currently means memcpying the entire queue contents
 * which is suck-licious
 * */
int
cf_queue_push(cf_queue *q, void *ptr)
{
	/* FIXME arg check - and how do you do that, boyo? Magic numbers? */

	/* FIXME error */
	if (0 != pthread_mutex_lock(&q->LOCK))
		return(-1);

	/* Check queue length */
	if (CF_Q_SZ(q) == q->allocsz) {
		/* resize is a pain for circular buffers */
		if (0 != cf_queue_resize(q, q->allocsz + CF_QUEUE_ALLOCSZ)) {
			pthread_mutex_unlock(&q->LOCK);
			return(-1);
		}
	}

	// todo: if queues are power of 2, this can be a shift
	memcpy(CF_Q_ELEM_PTR(q,q->write_offset), ptr, q->elementsz);
	q->write_offset++;
	if (q->write_offset & 0x80000000) cf_queue_unwrap(q);
	
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
		tp.tv_nsec += (ms_wait % 1000) * 1000000;
		if (tp.tv_nsec > 1000000000) {
			tp.tv_nsec -= 1000000000;
			tp.tv_sec++;
		}
	}

	/* FIXME error checking */
	/* Note that we apparently have to use a while() loop.  Careful reading
	 * of the pthread_cond_signal() documentation says that AT LEAST ONE
	 * waiting thread will be awakened... */
	while (CF_Q_EMPTY(q)) {
		if (CF_QUEUE_FOREVER == ms_wait) {
			pthread_cond_wait(&q->CV, &q->LOCK);
		}
		else if (CF_QUEUE_NOWAIT == ms_wait) {
			pthread_mutex_unlock(&q->LOCK);
			return(CF_QUEUE_EMPTY);
		}
		else {
			pthread_cond_timedwait(&q->CV, &q->LOCK, &tp);
			if (CF_Q_EMPTY(q)) {
				pthread_mutex_unlock(&q->LOCK);
				return(CF_QUEUE_EMPTY);
			}
		}
	}

	memcpy(buf, CF_Q_ELEM_PTR(q,q->read_offset), q->elementsz);
	q->read_offset++;
	
	// interesting idea - this probably keeps the cache fresher
	if (q->read_offset == q->write_offset) {
		q->read_offset = q->write_offset = 0;
	}

	/* FIXME blow a gasket */
	if (0 != pthread_mutex_unlock(&q->LOCK)) {
		fprintf(stderr, "unlock failed\n");
		return(-1);
	}

	return(0);
}


static void
cf_queue_delete_offset(cf_queue *q, uint index)
{
	index %= q->allocsz;
	uint r_index = q->read_offset % q->allocsz;
	uint w_index = q->write_offset % q->allocsz;
	
	// assumes index is validated!
	if (index == r_index) {
		q->read_offset++;
		return;
	}
	if (w_index && (index == w_index - 1)) {
		q->write_offset--;
		return;
	}
	// and the memory copy is overlapping, so must use memmove
	if (index > r_index) {
		memmove( &q->queue[ (r_index + 1) * q->elementsz ],
			     &q->queue[ r_index * q->elementsz ],
				 (index - r_index) * q->elementsz );
		q->read_offset++;
		return;
	}
	
	if (index < w_index) {
		memmove( &q->queue[ index * q->elementsz ],
			     &q->queue[ (index + 1) * q->elementsz ],
			     (w_index - index - 1) * q->elementsz);
		q->write_offset--;
		return;
	}
	
	D("QUEUE_DELETE_OFFSET: FAIL FAIL FAIL FAIL");

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

	if (CF_Q_SZ(q)) {
		
		// it would be faster to have a local variable to hold the index,
		// and do it in bytes or something, but a delete
		// will change the read and write offset, so this is simpler for now
		// can optimize if necessary later....
		
		for (uint i = q->read_offset ; 
			 i < q->write_offset ;
			 i++)
		{
			
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);
			
			// rv == 0 i snormal case, just increment to next point
			if (rv == -1) {
				break; // found what it was looking for
			}
			else if (rv == -2) { // delete!
				cf_queue_delete_offset(q, i);
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
	
	if (CF_Q_SZ(q)) {

		for (uint i = q->read_offset ; 
			 i < q->write_offset ;
			 i++)
		{
			
			int rv = memcmp(CF_Q_ELEM_PTR(q,i), buf, q->elementsz);
			
			if (rv == 0) { // delete!
				cf_queue_delete_offset(q, i);
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

//
// Test code
// it's always really annoying to have a queue that malfunctions in some small way
// so best to have a pretty serious torture test suite
//


#define TEST1_SZ 400
#define TEST1_INTERVAL 10

void *
cf_queue_test_1_write(void *arg)
{
	cf_queue *q = (cf_queue *) arg;
	
	for (int i=0; i<TEST1_SZ;i++) {
		
		usleep(TEST1_INTERVAL * 1000);
		
		int rv;
		rv = cf_queue_push(q, &i);
		if (0 != rv) {
			fprintf(stderr, "queue push failed: error %d",rv);
			return((void *)-1);
		}
	}
	
	return((void *)0);
	
}	

void *
cf_queue_test_1_read(void *arg)
{
	cf_queue *q = (cf_queue *) arg;
	
	for (int i=0;i<TEST1_SZ;i++) {
		
		// sleep twice as long as the inserter, to test overflow
		usleep(TEST1_INTERVAL * 1000 * 2);
		
		int  v = -1;
		int rv = cf_queue_pop(q, &v, CF_QUEUE_FOREVER);
		if (rv != CF_QUEUE_OK) {
			fprintf(stderr, "cf_queue_test1: pop error %d",rv);
			return((void *) -1);
		}
		if (v != i) {
			fprintf(stderr, "cf_queue_test1: pop value error: %d should be %d",v,i);
			return((void *) -1);
		}
		
	}
	return((void *) 0);	
}


int
cf_queue_test_1()
{
	pthread_t 	write_th;
	pthread_t     read_th;
	cf_queue    *q;
	
	q = cf_queue_create(sizeof(int));
	
	pthread_create( & write_th, 0, cf_queue_test_1_write, q);
	
	pthread_create( & read_th, 0, cf_queue_test_1_read, q);
	
	void *th_return;
	
	if (0 != pthread_join(write_th, &th_return)) {
		fprintf(stderr, "queue test 1: could not join1 %d\n",errno);
		return(-1);
	}
	
	if (0 != th_return) {
		fprintf(stderr, "queue test 1: returned error %p\n",th_return);
		return(-1);
	}
	
	if (0 != pthread_join(read_th, &th_return)) {
		fprintf(stderr, "queue test 1: could not join2 %d\n",errno);
		return(-1);
	}
	
	if (0 != th_return) {
		fprintf(stderr, "queue test 1: returned error 2 %p\n",th_return);
		return(-1);
	}
	
	cf_queue_destroy(q);
	
	return(0);
}

int
cf_queue_test()
{
	if (0 != cf_queue_test_1()) {
		fprintf(stderr, "CF QUEUE TEST ONE FAILED\n");
		return(-1);
	}
	
	return(0);
}

