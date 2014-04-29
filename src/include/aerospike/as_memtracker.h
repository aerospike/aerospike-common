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

struct as_memtracker_hooks_s;
typedef struct as_memtracker_hooks_s as_memtracker_hooks;

struct as_memtracker_s;
typedef struct as_memtracker_s as_memtracker;

/**
 * The interface which all memtrackers should implement.
 */
struct as_memtracker_hooks_s {
    /**
     * The destroy should free resources associated with the memtracker's source.
     * The destroy should not free the memtracker itself.
     */
    int (* destroy)(as_memtracker *);

    bool (* reserve)(const as_memtracker *, const uint32_t);
    bool (* release)(const as_memtracker *, const uint32_t);
    bool (* reset)(const as_memtracker *);
};

/**
 * Logger handle
 */
struct as_memtracker_s {
    bool                    is_malloc;
    void *                  source;
    const as_memtracker_hooks * hooks;
};

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated memtracker
 */
as_memtracker * as_memtracker_init(as_memtracker * memtracker, void * source, const as_memtracker_hooks * hooks);

/**
 * Heap allocate and initialize a memtracker
 */
as_memtracker * as_memtracker_new(void * source, const as_memtracker_hooks * hooks);


static inline void * as_memtracker_source(const as_memtracker * mt) {
    return (mt ? mt->source : NULL);
}

/**
 * Release resources associated with the memtracker.
 * Calls memtracker->destroy. If success and if this is a heap allocated
 * memtracker, then it will be freed.
 */
int as_memtracker_destroy(as_memtracker * memtracker);

/**
 * Reserve num_bytes bytes of memory
 */
bool as_memtracker_reserve(const as_memtracker * memtracker, const uint32_t num_bytes);

/**
 * Release num_bytes bytes of memory
 */
bool as_memtracker_release(const as_memtracker * memtracker, const uint32_t num_bytes);

/**
 * Release the entire reservation for the current thread
 */
bool as_memtracker_reset(const as_memtracker * memtracker);

