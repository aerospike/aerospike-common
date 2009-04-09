/*
 *  Citrusleaf Foundation
 *  include/timer.h - timer functionality
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once


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

/*
** Typedef for the timer callback
*/
typedef int (*cf_timer_fn) (void *udata);

struct cf_timer_element_s;

typedef struct cf_timer_element_s cf_timer_handle;

/* External functions */
extern cf_timer_handle *cf_timer_add(uint32_t ms, cf_timer_fn cb, void *udata);

extern void cf_timer_cancel(cf_timer_handle *hand);

/* need an init function to get the queues set up
*/
extern int cf_timer_init();


