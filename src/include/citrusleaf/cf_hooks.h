/* 
 * Copyright 2008-2014 Aerospike, Inc.
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

