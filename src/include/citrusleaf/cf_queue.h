/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <pthread.h>
#include <citrusleaf/cf_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#ifndef CF_QUEUE_ALLOCSZ
#define CF_QUEUE_ALLOCSZ 64
#endif

#define CF_QUEUE_OK 0
#define CF_QUEUE_ERR -1
#define CF_QUEUE_EMPTY -2
#define CF_QUEUE_NOMATCH -3 // used in cf_queue_priority_reduce_pop

// mswait < 0 wait forever
// mswait == 0 wait not at all
// mswait > 0 wait that number of ms
#define CF_QUEUE_FOREVER -1
#define CF_QUEUE_NOWAIT 0

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef int (*cf_queue_reduce_fn) (void *buf, void *udata);

/**
 * cf_queue
 * A queue 
 */
struct cf_queue_s {
    bool            threadsafe;     // sometimes it's good to live dangerously
    unsigned int    allocsz;        // number of queue elements currently allocated
    unsigned int    write_offset;   // 0 offset is first queue element.
                                    // write is always greater than or equal to read
    unsigned int    read_offset;    // 
    size_t          elementsz;      // number of bytes in an element
    pthread_mutex_t LOCK;           // the mutex lock
    pthread_cond_t  CV;             // the condvar
    byte *          queue;          // the actual bytes that make up the queue
};

typedef struct cf_queue_s cf_queue;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_queue * cf_queue_create(size_t elementsz, bool threadsafe);

void cf_queue_destroy(cf_queue *q);

/**
 * Get the number of elements currently in the queue
 */
int cf_queue_sz(cf_queue *q);
	
/**
 * Always pushes to the end of the queue
 */
int cf_queue_push(cf_queue *q, void *ptr);

/**
 * Push element on the queue only if size < limit.
 */
bool cf_queue_push_limit(cf_queue *q, void *ptr, uint limit);

/**
 * Same as cf_queue_push() except it's a no-op if element is already queued.
 */
int cf_queue_push_unique(cf_queue *q, void *ptr);
	
/**
 * Push head goes to the front, which currently means memcpying the entire queue contents.
 */
int cf_queue_push_head(cf_queue *q, void *ptr);

/**
 * POP pops from the end of the queue, which is the most efficient
 * But understand this makes it LIFO, the least fair of queues
 * Elements added at the very beginning might not make it out
 */
int cf_queue_pop(cf_queue *q, void *buf, int mswait);

/**
 * Run the entire queue, calling the callback, with the lock held.
 * You can return values in the callback to cause deletes.
 * Great for purging dying stuff out of a queue synchronously.
 * 
 * return -2 from the callback to trigger a delete
 * return -1 stop iterating the queue
 * return 0 for success
 */
int cf_queue_reduce(cf_queue *q, cf_queue_reduce_fn cb, void *udata);

/**
 * Run the entire queue in reverse order, calling the callback, with the lock held.
 * You can return values in the callback to cause deletes.
 * Great for purging dying stuff out of a queue synchronously.
 *
 * return -2 from the callback to trigger a delete
 * return -1 stop iterating the queue
 * return 0 for success
 */
int cf_queue_reduce_reverse(cf_queue *q, cf_queue_reduce_fn cb, void *udata);

/**
 * The most common reason to want to 'reduce' is delete - so provide
 * a simple delete function
 */
int cf_queue_delete(cf_queue *q, void *buf, bool only_one);

/**
 * Delete all items in queue.
 */
int cf_queue_delete_all(cf_queue *q);

void cf_queue_delete_offset(cf_queue *q, uint index);

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define CF_Q_SZ(__q) (__q->write_offset - __q->read_offset)

#define CF_Q_EMPTY(__q) (__q->write_offset == __q->read_offset)

/**
 * todo: maybe it's faster to keep the read and write offsets in bytes,
 * to avoid the extra multiply?
 */
#define CF_Q_ELEM_PTR(__q, __i) (&__q->queue[ (__i % __q->allocsz) * __q->elementsz ] )

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
