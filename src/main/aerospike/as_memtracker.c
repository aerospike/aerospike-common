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

#include <aerospike/as_memtracker.h>
#include <aerospike/as_util.h>

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated memtracker
 */
as_memtracker * as_memtracker_init(as_memtracker * memtracker, void * source, const as_memtracker_hooks * hooks) {
    if ( memtracker == NULL ) return memtracker;
    memtracker->source = source;
    memtracker->hooks = hooks;
    return memtracker;
}

/**
 * Heap allocate and initialize a memtracker
 */
as_memtracker * as_memtracker_new(void * source, const as_memtracker_hooks * hooks) {
    as_memtracker * memtracker = (as_memtracker *) cf_malloc(sizeof(as_memtracker));
    if (!memtracker) return memtracker;
    memtracker->source = source;
    memtracker->hooks = hooks;
    return memtracker;
}

/**
 * Release resources associated with the memtracker.
 * Calls memtracker->destroy. If success and if this is a heap allocated
 * memtracker, then it will be freed.
 */
int as_memtracker_destroy(as_memtracker * memtracker) {
    int rc = as_util_hook(destroy, 1, memtracker);
    if ( rc == 0 && memtracker->is_malloc ) {
        cf_free(memtracker);
    }
    return rc;
}

/**
 * For most purposes, you should use the macros:
 *   - as_memtracker_reserve(memtracker, num_bytes)
 *   - as_memtracker_release(memtracker, num_bytes)
 */
bool as_memtracker_reserve(const as_memtracker * memtracker, const uint32_t num_bytes) {
    return as_util_hook(reserve, false, memtracker, num_bytes);
}

bool as_memtracker_release(const as_memtracker * memtracker, const uint32_t num_bytes) {
    return as_util_hook(release, false, memtracker, num_bytes);
}

bool as_memtracker_reset(const as_memtracker * memtracker) {
    return as_util_hook(reset, false, memtracker);
}
