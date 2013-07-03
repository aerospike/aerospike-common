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
    as_memtracker * memtracker = (as_memtracker *) malloc(sizeof(as_memtracker));
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
        free(memtracker);
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
