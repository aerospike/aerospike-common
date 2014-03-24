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

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
 * TYPES
 *****************************************************************************/

struct as_timer_hooks_s;
typedef struct as_timer_hooks_s as_timer_hooks;

struct as_timer_s;
typedef struct as_timer_s as_timer;

/**
 * The interface which all timer should implement.
 */
struct as_timer_hooks_s {
    /**
     * The destroy should free resources associated with the timer's source.
     * The destroy should not free the timer itself.
     */
    int      (* destroy)(as_timer *);
    bool     (* timedout)(const as_timer *);
	uint64_t (* timeslice)(const as_timer *);
};

/**
 * Timer handle
 */
struct as_timer_s {
    bool                    is_malloc;
    void *                  source;
    const as_timer_hooks  * hooks;
};

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated timer
 */
as_timer * as_timer_init(as_timer * timer, void * source, const as_timer_hooks * hooks);

/**
 * Heap allocate and initialize a timer
 */
as_timer * as_timer_new(void * source, const as_timer_hooks * hooks);


inline void * as_timer_source(const as_timer * tt) {
    return (tt ? tt->source : NULL);
}

/**
 * Release resources associated with the timer.
 * Calls timer->destroy. If success and if this is a heap allocated
 * timer, then it will be freed.
 */
int as_timer_destroy(as_timer * timer);

/**
 * true if timer has timedout
 */
bool as_timer_timedout(const as_timer * timer);

/**
 * returns timeslice assigned for this timer
 */
uint64_t as_timer_timeslice(const as_timer * timer);
