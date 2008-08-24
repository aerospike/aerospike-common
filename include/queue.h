/*
 *  Citrusleaf Foundation
 *  include/queue.h - queue structures
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once



/* SYNOPSIS
 * Queue
 */


/* cf_queue
 * A queue */
#define CF_QUEUE_ALLOCSZ 64
struct cf_queue_t {
	uint16_t allocsz, utilsz;
	pthread_mutex_t LOCK;
	pthread_cond_t CV;
	void *queue[];
};
typedef struct cf_queue_t cf_queue;


/* External functions */
extern cf_queue *cf_queue_create();
extern int cf_queue_push(cf_queue *q, void *ptr);
extern void *cf_queue_pop(cf_queue *q, int block);
