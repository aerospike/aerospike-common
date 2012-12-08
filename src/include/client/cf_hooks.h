/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once


typedef struct cf_mutex_hooks_s {
	void *(*alloc)(void);		// Allocate and initialize new lock.
	void (*free)(void *lock);	// Release all storage held in 'lock'.
	int (*lock)(void *lock);	// Acquire an already-allocated lock at 'lock'.
	int (*unlock)(void *lock);	// Release a lock at 'lock'.
} cf_mutex_hooks;

extern cf_mutex_hooks* g_mutex_hooks;

static inline void cf_hook_mutex(cf_mutex_hooks *hooks) {
	g_mutex_hooks = hooks;
}

static inline void * cf_hooked_mutex_alloc() {
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
