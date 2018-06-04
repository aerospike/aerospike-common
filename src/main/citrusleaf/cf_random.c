/*
 * Copyright 2008-2018 Aerospike, Inc.
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
#include <citrusleaf/cf_random.h>
#include <fcntl.h>
#include <openssl/rand.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#if !defined ENHANCED_ALLOC
#include <aerospike/as_log_macros.h>
#endif

#define SEED_SZ 64
static uint8_t rand_buf[1024 * 8];
static uint32_t rand_buf_off = 0;
static int  seeded = 0;
static pthread_mutex_t rand_buf_lock = PTHREAD_MUTEX_INITIALIZER;

#if defined(__linux__) || defined(__FreeBSD__)
#include <unistd.h>

int
cf_rand_reload()
{
	if (seeded == 0) {
		int rfd = open("/dev/urandom", O_RDONLY);
		int rsz = (int)read(rfd, rand_buf, SEED_SZ);
		if (rsz < SEED_SZ) {
#if !defined ENHANCED_ALLOC
			as_log_error("Failed to seed random number generator");
#endif
			return(-1);
		}
		close(rfd);
		RAND_seed(rand_buf, rsz);
		seeded = 1;
	}

	if (1 != RAND_bytes(rand_buf, sizeof(rand_buf))) {
#if !defined ENHANCED_ALLOC
		as_log_error("Failed to reload random buffer");
#endif
		return(-1);
	}

	rand_buf_off = sizeof(rand_buf);
	return 0;
}

#elif defined (__APPLE__)

int
cf_rand_reload()
{
    if (seeded == 0) {
        arc4random_stir();
        seeded = 1;
    }
    
    arc4random_buf(rand_buf, sizeof(rand_buf));
    rand_buf_off = sizeof(rand_buf);
    return 0;
}

#elif defined (_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int
cf_rand_reload()
{
	// Acquire/Release context every buffer reload.
	HCRYPTPROV hProvider;

	if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
#if !defined ENHANCED_ALLOC
		as_log_error("Failed to seed random number generator");
#endif
		return -1;
	}

	if (!CryptGenRandom(hProvider, sizeof(rand_buf), rand_buf)) {
#if !defined ENHANCED_ALLOC
		as_log_error("Failed to reload random buffer");
#endif
		CryptReleaseContext(hProvider, 0);
		return -1;
	}

	CryptReleaseContext(hProvider, 0);
	rand_buf_off = sizeof(rand_buf);
	return 0;
}
#endif

int
cf_get_rand_buf(uint8_t *buf, int len)
{
    if ((uint32_t)len >= sizeof(rand_buf))  return(-1);
    
    pthread_mutex_lock(&rand_buf_lock);
    
    if (rand_buf_off < (uint32_t)len ) {
        if (-1 == cf_rand_reload()) {
            pthread_mutex_unlock(&rand_buf_lock);
            return(-1);
        }
    }

    rand_buf_off -= len;
    memcpy(buf, &rand_buf[rand_buf_off] ,len);

    pthread_mutex_unlock(&rand_buf_lock);   

    return(0);  
}

uint64_t
cf_get_rand64()
{
    pthread_mutex_lock(&rand_buf_lock);
    if (rand_buf_off < sizeof(uint64_t) ) {
        if (-1 == cf_rand_reload()) {
            pthread_mutex_unlock(&rand_buf_lock);
            return(0);
        }
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
        if (-1 == cf_rand_reload()) {
            pthread_mutex_unlock(&rand_buf_lock);
            return(0);
        }
    }
    
    rand_buf_off -= sizeof(uint64_t);
    uint64_t r = *(uint64_t *) (&rand_buf[rand_buf_off]);
    pthread_mutex_unlock(&rand_buf_lock);
    return((uint32_t)r);
}
