/*
 *      cf_queue_priority.h
 *
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * A simple priority queue implementation, which is simply a set of queues
 * underneath
 * This currently doesn't support 'delete' and 'reduce' functionality
 */
#pragma once
#include "cf_queue.h"

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define CF_QUEUE_PRIORITY_HIGH 1
#define CF_QUEUE_PRIORITY_MEDIUM 2
#define CF_QUEUE_PRIORITY_LOW 3

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct cf_queue_priority_s cf_queue_priority;

struct cf_queue_priority_s {
    bool            threadsafe;
    cf_queue *      low_q;
    cf_queue *      medium_q;
    cf_queue *      high_q;
#ifdef EXTERNAL_LOCKS
    void *          LOCK;
#else   
    pthread_mutex_t LOCK;
    pthread_cond_t  CV;
#endif
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_queue_priority *cf_queue_priority_create(size_t elementsz, bool threadsafe);
void cf_queue_priority_destroy(cf_queue_priority *q);
int cf_queue_priority_push(cf_queue_priority *q, void *ptr, int pri);
int cf_queue_priority_pop(cf_queue_priority *q, void *buf, int mswait);
int cf_queue_priority_sz(cf_queue_priority *q);

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define CF_Q_PRI_EMPTY(__q) (CF_Q_EMPTY(__q->low_q) && CF_Q_EMPTY(__q->medium_q) && CF_Q_EMPTY(__q->high_q))
