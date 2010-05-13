/*
 *  Citrusleaf Foundation
 *  src/string.c - string helper functions
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "cf.h"

#include <openssl/rand.h>

#define SEED_SZ 64
static uint8_t rand_buf[1024 * 8];
static uint rand_buf_off = 0;
static int	seeded = 0;
static pthread_mutex_t rand_buf_lock = PTHREAD_MUTEX_INITIALIZER;

uint64_t
cf_get_rand64()
{
	pthread_mutex_lock(&rand_buf_lock);
	if (rand_buf_off < sizeof(uint64_t) ) {
		if (seeded == 0) {
			int rfd = open("/dev/urandom",	O_RDONLY);
			int rsz = read(rfd, rand_buf, SEED_SZ);
			if (rsz < SEED_SZ) {
				fprintf(stderr, "warning! can't seed random number generator");
				pthread_mutex_unlock(&rand_buf_lock);
				return(0);
			}
			close(rfd);
			RAND_seed(rand_buf, rsz);
			seeded = 1;
		}
		if (1 != RAND_bytes(rand_buf, sizeof(rand_buf))) {
			fprintf(stderr, "RAND_bytes not so happy.\n");
			pthread_mutex_unlock(&rand_buf_lock);
			return(0);
		}
		rand_buf_off = sizeof(rand_buf);
	}
	
	rand_buf_off -= sizeof(uint64_t);
	uint64_t r = *(uint64_t *) (&rand_buf[rand_buf_off]);
	pthread_mutex_unlock(&rand_buf_lock);
	return(r);
}

uint32_t
cf_get_rand32()
{
	pthread_mutex_lock(&rand_buf_lock);
	if (rand_buf_off < sizeof(uint64_t) ) {
		if (seeded == 0) {
			int rfd = open("/dev/urandom",	O_RDONLY);
			int rsz = read(rfd, rand_buf, SEED_SZ);
			if (rsz < SEED_SZ) {
				fprintf(stderr, "warning! can't seed random number generator");
				pthread_mutex_unlock(&rand_buf_lock);
				return(0);
			}
			close(rfd);
			RAND_seed(rand_buf, rsz);
			seeded = 1;
		}
		if (1 != RAND_bytes(rand_buf, sizeof(rand_buf))) {
			fprintf(stderr, "RAND_bytes not so happy.\n");
			pthread_mutex_unlock(&rand_buf_lock);
			return(0);
		}
		rand_buf_off = sizeof(rand_buf);
	}
	
	rand_buf_off -= sizeof(uint64_t);
	uint64_t r = *(uint64_t *) (&rand_buf[rand_buf_off]);
	pthread_mutex_unlock(&rand_buf_lock);
	return(r);
}

