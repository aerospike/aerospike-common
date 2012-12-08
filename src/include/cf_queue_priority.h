/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

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
