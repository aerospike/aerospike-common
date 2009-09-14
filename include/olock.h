/*
 * An object lock system allows fewer locks to be created
 *   
 *
 * Copywrite 2009 Brian Bulkowski
 * All rights reserved
 */

#pragma once
 
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "cf.h"


typedef struct olock_s {
	uint32_t n_locks;
	uint32_t mask;
	pthread_mutex_t   locks[];
} olock;

void olock_lock(olock *ol, cf_digest *d);
void olock_vlock(olock *ol, cf_digest *d, pthread_mutex_t **vlock);
void olock_unlock(olock *ol, cf_digest *d);
olock *olock_create(uint32_t n_locks,  bool mutex); 
void olock_destroy(olock *o);

