/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#include "client/cf_queue_priority.h"
#include "client/cf_log_internal.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

/******************************************************************************
 * MACROS
 ******************************************************************************/

#ifdef EXTERNAL_LOCKS
#include "cf_hooks.h"
#define QUEUE_LOCK(_q)      if ( _q->threadsafe ) { cf_hooked_mutex_lock(_q->LOCK); }
#define QUEUE_UNLOCK(_q)    if ( _q->threadsafe ) { cf_hooked_mutex_unlock(_q->LOCK); }
#else
#define QUEUE_LOCK(_q)      if ( _q->threadsafe ) { pthread_mutex_lock(&_q->LOCK); }
#define QUEUE_UNLOCK(_q)    if ( _q->threadsafe ) { pthread_mutex_unlock(&_q->LOCK); }
#endif

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/


cf_queue_priority * cf_queue_priority_create(size_t elementsz, bool threadsafe) {
    cf_queue_priority *q = malloc(sizeof(cf_queue_priority));
    if (!q) return(0);
    
    q->threadsafe = threadsafe;
    q->low_q = cf_queue_create(elementsz, false);
    if (!q->low_q)      goto Fail1;
    q->medium_q = cf_queue_create(elementsz, false);
    if (!q->medium_q)   goto Fail2;
    q->high_q = cf_queue_create(elementsz, false);
    if (!q->high_q)     goto Fail3;
    
    if (threadsafe == false)
        return(q);
#ifdef EXTERNAL_LOCKS
    q->LOCK = cf_hooked_mutex_alloc();
    if (!q->LOCK ) goto Fail5;
#else   
    if (0 != pthread_mutex_init(&q->LOCK, NULL))
        goto Fail4;

    if (0 != pthread_cond_init(&q->CV, NULL))
        goto Fail5;
#endif // EXTERNAL_LOCKS
    
    return(q);
    
Fail5:  
#ifdef EXTERNAL_LOCKS
    cf_hooked_mutex_free(q->LOCK);
#else
    pthread_mutex_destroy(&q->LOCK);
Fail4:  
#endif // EXTERNAL_LOCKS
    cf_queue_destroy(q->high_q);
Fail3:  
    cf_queue_destroy(q->medium_q);
Fail2:  
    cf_queue_destroy(q->low_q);
Fail1:  
    free(q);
    return(0);
}

void cf_queue_priority_destroy(cf_queue_priority *q) {
    cf_queue_destroy(q->high_q);
    cf_queue_destroy(q->medium_q);
    cf_queue_destroy(q->low_q);
    if (q->threadsafe) {
#ifdef EXTERNAL_LOCKS
        cf_hooked_mutex_free(q->LOCK);
#else
        pthread_mutex_destroy(&q->LOCK);
        pthread_cond_destroy(&q->CV);
#endif // EXTERNAL_LOCKS
    }
    free(q);
}

int cf_queue_priority_push(cf_queue_priority *q, void *ptr, int pri) {
    
    QUEUE_LOCK(q);
    
    int rv;
    if (pri == CF_QUEUE_PRIORITY_HIGH)
        rv = cf_queue_push(q->high_q, ptr);
    else if (pri == CF_QUEUE_PRIORITY_MEDIUM)
        rv = cf_queue_push(q->medium_q, ptr);
    else if (pri == CF_QUEUE_PRIORITY_LOW)
        rv = cf_queue_push(q->low_q, ptr);
    else {
        rv = -1;
    }

#ifndef EXTERNAL_LOCKS
    if (rv == 0 && q->threadsafe)
        pthread_cond_signal(&q->CV);
#endif 

    QUEUE_UNLOCK(q);
        
    return(rv); 
}

int  cf_queue_priority_pop(cf_queue_priority *q, void *buf, int ms_wait) {
    QUEUE_LOCK(q);

    struct timespec tp;
    if (ms_wait > 0) {
#ifdef OSX
        uint64_t curms = cf_getms(); // using the cl generic functions defined in cf_clock.h. It is going to have slightly less resolution than the pure linux version
        tp.tv_sec = (curms + ms_wait)/1000;
        tp.tv_nsec = (ms_wait %1000) * 1000000;
#else // linux
        clock_gettime( CLOCK_REALTIME, &tp); 
        tp.tv_sec += ms_wait / 1000;
        tp.tv_nsec += (ms_wait % 1000) * 1000000;
        if (tp.tv_nsec > 1000000000) {
            tp.tv_nsec -= 1000000000;
            tp.tv_sec++;
        }
#endif
    }

    if (q->threadsafe) {
#ifdef EXTERNAL_LOCKS
        if (CF_Q_PRI_EMPTY(q)) {
            QUEUE_UNLOCK(q);
            return -1;
        }
#else
        while (CF_Q_PRI_EMPTY(q)) {
            if (CF_QUEUE_FOREVER == ms_wait) {
                pthread_cond_wait(&q->CV, &q->LOCK);
            }
            else if (CF_QUEUE_NOWAIT == ms_wait) {
                pthread_mutex_unlock(&q->LOCK);
                return(CF_QUEUE_EMPTY);
            }
            else {
                pthread_cond_timedwait(&q->CV, &q->LOCK, &tp);
                if (CF_Q_PRI_EMPTY(q)) {
                    pthread_mutex_unlock(&q->LOCK);
                    return(CF_QUEUE_EMPTY);
                }
            }
        }
#endif //EXERNAL_LOCKS
    }
    
    int rv;
    if (CF_Q_SZ(q->high_q))
        rv = cf_queue_pop(q->high_q, buf, 0);
    else if (CF_Q_SZ(q->medium_q))
        rv = cf_queue_pop(q->medium_q, buf, 0);
    else if (CF_Q_SZ(q->low_q))
        rv = cf_queue_pop(q->low_q, buf, 0);
    else rv = CF_QUEUE_EMPTY;
        
    QUEUE_UNLOCK(q);

        
    return(rv);
}

int cf_queue_priority_sz(cf_queue_priority *q) {
    int rv = 0;
    QUEUE_LOCK(q);
    rv += cf_queue_sz(q->high_q);
    rv += cf_queue_sz(q->medium_q);
    rv += cf_queue_sz(q->low_q);
    QUEUE_UNLOCK(q);
    return(rv);
}
