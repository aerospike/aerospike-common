/*
 *  Citrusleaf Foundation
 *  src/olock.c - object locks
 *
 * The object lock system gives a list 
 *
 *  Copyright 2009 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "cf.h"

#include <signal.h>

// TALK ABOUT HACKS
// an interesting detail: since this digest is used to choose among
// servers, you must use different bits to choose which OLOCK
// this is mediated in the application :-(
//
// need a fancier architecture to share the hash bits, hacking it for now
// (please laugh when you read this code in 2020 and it still works the same way)
// -brian

//
// ASSUMES d is DIGEST and ol is OLOCK *
//

#define OLOCK_HASH(__ol, __d) ( ( (__d->digest[10] << 8) | (__d->digest[11]) ) & __ol->mask )


void
olock_lock(olock *ol, cf_digest *d)
{
	uint32_t n = OLOCK_HASH(ol, d);

//	if (0 != cf_mutex_timedlock( &ol->locks[ n & ol->mask ], 2000 ) ) {
	pthread_mutex_lock( &ol->locks[ n ] );
	
//		fprintf(stderr, "olock lock failed %d\n",errno);
//		raise(SIGINT);

	return;
}

void
olock_vlock(olock *ol, cf_digest *d, pthread_mutex_t **vlock)
{
	uint32_t n = OLOCK_HASH(ol, d);
	
	*vlock = &ol->locks[ n ];

//	int rv = cf_mutex_timedlock( *vlock, 2000 );
//	if (rv != 0 ) {
//		fprintf(stderr, "mutex olock vlock fail: rv %d errno %d\n",rv,errno);
//		raise(SIGINT);
//	}

	if (0 != pthread_mutex_lock( *vlock )) {
		fprintf(stderr, "olock vlock failed\n");
	}
	return;
}


void
olock_unlock(olock *ol, cf_digest *d)
{
	uint32_t n = OLOCK_HASH(ol, d);

//	fprintf(stderr, "-%p", &ol->locks[ n & ol->mask ] );
	if (0 != pthread_mutex_unlock( &ol->locks[ n ] )) {
		fprintf(stderr, "olock unlock failed %d\n",errno);
	}
	return;
}

olock *
olock_create(uint32_t n_locks,  bool mutex) 
{
	
	olock *ol = cf_malloc( sizeof (olock) + (sizeof(pthread_mutex_t) * n_locks));
	if (!ol)	return(0);
	
	uint32_t mask = n_locks - 1;
	if ((mask & n_locks) != 0) {
		fprintf(stderr, "olock: make sure your number of locks is a power of 2, n_locks aint\n");
		return(0);
	}
	
	ol->n_locks = n_locks;
	ol->mask = mask;
	for (int i=0;i<n_locks;i++) {
		if (mutex)
			pthread_mutex_init(& ol->locks[i], 0 );
		else
			fprintf(stderr, "olock: todo add reader writer locks\n");
	}
	return(ol);
}

void
olock_destroy(olock *ol)
{
	for (int i=0;i<ol->n_locks;i++)
		pthread_mutex_destroy( & ol->locks[i] );
	cf_free(ol);
}

