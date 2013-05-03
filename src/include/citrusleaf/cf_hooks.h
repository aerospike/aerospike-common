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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cf_mutex_hooks_s {
    // Allocate and initialize new lock.
    void *(*alloc)(void);
    // Release all storage held in 'lock'.
    void (*free)(void *lock);
    // Acquire an already-allocated lock at 'lock'.
    int (*lock)(void *lock);
    // Release a lock at 'lock'.
    int (*unlock)(void *lock);
} cf_mutex_hooks;

extern cf_mutex_hooks* g_mutex_hooks;

static inline void cf_hook_mutex(cf_mutex_hooks *hooks) {
    g_mutex_hooks = hooks;
}

static inline void *  cf_hooked_mutex_alloc() {
    return g_mutex_hooks ? g_mutex_hooks->alloc() : 0;
}

static inline void cf_hooked_mutex_free(void *lock) {
    if (lock) {
        g_mutex_hooks->free(lock);
    }
}

static inline int cf_hooked_mutex_lock(void *lock) {
    return lock ? g_mutex_hooks->lock(lock) : 0;
}

static inline int cf_hooked_mutex_unlock(void *lock) {
    return lock ? g_mutex_hooks->unlock(lock) : 0;
}

#ifdef __cplusplus
}
#endif

