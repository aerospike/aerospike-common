/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdarg.h>
#include <stdio.h>

#include <citrusleaf/alloc.h>

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
    as_timer * timer = (as_timer *) cf_malloc(sizeof(as_timer));
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
        cf_free(timer);
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
