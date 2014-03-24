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

#include <stdarg.h>
#include <stdio.h>

#include <aerospike/as_timer.h>
#include <aerospike/as_util.h>

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated timer
 */
as_timer * as_timer_init(as_timer * timer, void * source, const as_timer_hooks * hooks) {
    if ( timer == NULL ) return timer;
    timer->source = source;
    timer->hooks = hooks;
    return timer;
}

/**
 * Heap allocate and initialize a timer
 */
as_timer * as_timer_new(void * source, const as_timer_hooks * hooks) {
    as_timer * timer = (as_timer *) malloc(sizeof(as_timer));
    if (!timer) return timer;
    timer->source = source;
    timer->hooks = hooks;
    return timer;
}

/**
 * Release resources associated with the timer.
 * Calls timer->destroy. If success and if this is a heap allocated
 * timer, then it will be freed.
 */
int as_timer_destroy(as_timer * timer) {
    int rc = as_util_hook(destroy, 1, timer);
    if ( rc == 0 && timer->is_malloc ) {
        free(timer);
    }
    return rc;
}

bool as_timer_timedout(const as_timer * timer) {
	if (timer == NULL) return 0;
	return as_util_hook(timedout, false, timer);
}

uint64_t as_timer_timeslice(const as_timer * timer) {
	if (timer == NULL) return 0;
	return as_util_hook(timeslice, false, timer);
}
