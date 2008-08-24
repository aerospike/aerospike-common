/*
 *  Citrusleaf Foundation
 *  include/lock.h - locking operations
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include "atomic.h"



/* SYNOPSIS
 * Mellor-Crummey & Scott locks
 * http://www.cs.rochester.edu/research/synchronization/pseudocode/ss.html
 * enqueues for each locking thread a structure in a global queue
 * only spins on its own data structure
 * cache efficiency over test-and-sets */


/* cf_mcslock_qnode
 * A MCS lock queue node */
struct cf_mcslock_qnode_t {
	int locked;
	struct cf_mcslock_qnode_t *next;
};
typedef struct cf_mcslock_qnode_t cf_mcslock_qnode;


/* cf_mcslock
 * A MCS lock */
typedef struct { cf_mcslock_qnode *tail; } cf_mcslock;


/* cf_mcslock_init
 * Initialize a MCS lock */
static inline void
cf_mcslock_init(cf_mcslock *lock)
{
	lock->tail = NULL;

	return;
}


/* cf_mcslock_lock
 * Lock a MCS lock */
static inline void
cf_mcslock_lock(cf_mcslock *lock, cf_mcslock_qnode *qn)
{
	cf_mcslock_qnode *qp = NULL;

	qn->next = NULL;
	qn->locked = 1;
	CF_MEMORY_BARRIER_WRITE();

	qp = cf_atomic_fas(&lock->tail, qp);
	if (NULL != qp) {
		qp->next = qn;
		while (qn->locked)
			CF_MEMORY_BARRIER_READ();
	}

	CF_MEMORY_BARRIER();
	return;
}


/* cf_mcslock_unlock
 * Release a MCS lock */
static inline void
cf_mcslock_unlock(cf_mcslock *lock, cf_mcslock_qnode *qn)
{
	cf_mcslock_qnode *qt = qn->next;

	CF_MEMORY_BARRIER();

	if (NULL == qt) {
		if (qn == (cf_atomic_cas(&lock->tail, qn, NULL)))
			return;
		while (NULL == (qt = qn->next))
			CF_MEMORY_BARRIER_READ();
	}

	qt->locked = 0;
}

