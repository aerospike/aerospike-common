/*
 *  Citrusleaf Foundation
 *  include/atomic.h - atomic memory operations
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once

#include <stdint.h>

/* SYNOPSIS
 * Atomic memory operations
 * Memory barriers
 * > */


/* Atomic memory operations */

/* cf_atomic64
 * An implementation of an atomic 64-bit data type
 * NB: some older compilers may exhibit aliasing bugs that require this
 * definition to be wrapped in a structure */
typedef volatile int64_t cf_atomic64;


/* cf_atomic64_get
 * Read an atomic value */
#define cf_atomic64_get(a) (a)


/* cf_atomic64_set
 * Set an atomic value */
#define cf_atomic64_set(a, b) (*(a) = (b))


/* cf_atomic64_add
 * Atomic addition: add a value b into an atomic integer a, and return the
 * result */
static inline int64_t
cf_atomic64_add(cf_atomic64 *a, int64_t b)
{
	int64_t i = b;

	__asm__ __volatile__ ("lock; xaddq %0, %1" : "+r" (b), "+m" (*a) : : "memory");

	return(b + i);
}
#define cf_atomic64_incr(a) (cf_atomic64_add((a), 1))
#define cf_atomic64_decr(a) (cf_atomic64_add((a), -1))


/* cf_atomic64_cas
 * Compare-and-swap: test a value b against an atomic integer a; if they
 * are equal, store the value x into a, and return the initial value of a.
 * "Success" can be checked by comparing the returned value against b
 * NB: this is a strong memory barrier
 */
static inline int64_t
cf_atomic64_cas(cf_atomic64 *a, int64_t b, int64_t x)
{
	int64_t p;

	__asm__ __volatile__ ("lock; cmpxchgq %1,%2" : "=a"(p) : "q"(x), "m"(*(a)), "0"(b) : "memory");

	return(p);
}

#define cf_atomic_cas(_a, _b, _x) \
({  __typeof__(_b) __b = _b; \
	__asm__ __volatile__ ("lock; cmpxchgq %1,%2" : "=a"(__b) : "q"(_x), "m"(*(_a)), "0"(_b) : "memory"); \
	__b; \
})

/* cf_atomic64_fas
 * Fetch-and-swap: swap the values of  b and a */
static inline int64_t
cf_atomic64_fas(cf_atomic64 *a, cf_atomic64 *b)
{
	int64_t p;

	__asm__ __volatile__ ("lock; xchgq %0,%1" : "=r"(p) : "m"(*(a)), "0"(*(b)) : "memory");

	return(p);
}

#define cf_atomic_fas(_a, _b) \
({  __typeof__(_b) __b; \
	__asm__ __volatile__ ("lock; xchgq %0,%1" : "=r"(__b) : "m"(*(_a)), "0"(_b)); \
	__b; \
})


/* cf_atomic64_addunless
 * Increment-unless: test a value b against an atomic integer a.  If they
 * are NOT equal, add x to a, and return non-zero; if they ARE equal, return
 * zero */
static inline int64_t
cf_atomic64_addunless(cf_atomic64 *a, int64_t b, int64_t x)
{
	int64_t prior, cur;

	/* Get the current value of the atomic integer */
	cur = cf_atomic64_get(*a);

	for ( ;; ) {
		/* Check if the current value is equal to the criterion */
		if (cur == b)
			break;

		/* Attempt a compare-and-swap, which will return the value of cur
		 * prior to the operation */
		prior = cf_atomic64_cas(a, cur, cur + x);

		/* If prior and cur are equal, then the operation has succeeded;
		 * otherwise, set cur to prior and go around again */
		if (prior == cur)
			break;
		cur = prior;
	}

	return(cur != b);
}



/* Memory barriers */

/* CF_BARRIER
 * All preceding memory accesses must commit before any following accesses */
#define CF_MEMORY_BARRIER() __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")

/* CF_BARRIER_READ
 * All preceding memory accesses must commit before any following accesses */
#define CF_MEMORY_BARRIER_READ() CF_MEMORY_BARRIER()

/* CF_BARRIER_WRITE
 * All preceding memory accesses must commit before any following accesses */
#define CF_MEMORY_BARRIER_WRITE() __asm__ __volatile__ ("" : : : "memory")
