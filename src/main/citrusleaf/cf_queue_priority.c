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
#include <citrusleaf/cf_queue_priority.h>
#include <citrusleaf/cf_clock.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_queue_priority * cf_queue_priority_create(size_t elementsz, bool threadsafe)
{
    cf_queue_priority *q = cf_malloc(sizeof(cf_queue_priority));
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
    
    if (0 != pthread_mutex_init(&q->LOCK, NULL))
        goto Fail4;
	
    if (0 != pthread_cond_init(&q->CV, NULL))
        goto Fail5;
    
    return(q);
    
Fail5:
    pthread_mutex_destroy(&q->LOCK);
Fail4:
    cf_queue_destroy(q->high_q);
Fail3:
    cf_queue_destroy(q->medium_q);
Fail2:
    cf_queue_destroy(q->low_q);
Fail1:
    cf_free(q);
    return(0);
}

void cf_queue_priority_destroy(cf_queue_priority *q)
{
    cf_queue_destroy(q->high_q);
    cf_queue_destroy(q->medium_q);
    cf_queue_destroy(q->low_q);
    if (q->threadsafe) {
        pthread_mutex_destroy(&q->LOCK);
        pthread_cond_destroy(&q->CV);
    }
    cf_free(q);
}

int cf_queue_priority_push(cf_queue_priority *q, void *ptr, int pri)
{
    if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK)))
		return(-1);
    
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
	
    if (rv == 0 && q->threadsafe)
        pthread_cond_signal(&q->CV);
	
    if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK)))
        return(-1);
		
    return(rv);
}

int cf_queue_priority_pop(cf_queue_priority *q, void *buf, int ms_wait)
{
    if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK)))
		return(-1);
	
    struct timespec tp;
    if (ms_wait > 0) {
		cf_set_wait_timespec(ms_wait, &tp);
    }
	
    if (q->threadsafe) {
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
    }
    
    int rv;
    if (CF_Q_SZ(q->high_q))
        rv = cf_queue_pop(q->high_q, buf, 0);
    else if (CF_Q_SZ(q->medium_q))
        rv = cf_queue_pop(q->medium_q, buf, 0);
    else if (CF_Q_SZ(q->low_q))
        rv = cf_queue_pop(q->low_q, buf, 0);
    else rv = CF_QUEUE_EMPTY;
	
    if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK)))
        return(-1);
	
	
    return(rv);
}

int cf_queue_priority_sz(cf_queue_priority *q)
{
    int rv = 0;
    if (q->threadsafe)
        pthread_mutex_lock(&q->LOCK);
    rv += cf_queue_sz(q->high_q);
    rv += cf_queue_sz(q->medium_q);
    rv += cf_queue_sz(q->low_q);
    if (q->threadsafe)
        pthread_mutex_unlock(&q->LOCK);
    return(rv);
}

/**
 * Use this function to find an element to pop from the queue using a reduce
 * callback function. Have the callback function
 * return -1 when you know you want to pop the element immediately, returns -2
 * when the element is the best candidate for popping found so far but you want
 * to keep looking, and returns 0 when you are not interested in popping
 * the element. You then pop the best candidate you've found - either the
 * "-1 case" or the last "-2 case". If you have not found a suitable candidate,
 * CF_QUEUE_NOMATCH is returned.
 */
int cf_queue_priority_reduce_pop(cf_queue_priority *priority_q,  void *buf, cf_queue_reduce_fn cb, void *udata)
{
    if (NULL == priority_q)
        return(-1);
	
    if (priority_q->threadsafe && (0 != pthread_mutex_lock(&priority_q->LOCK)))
        return(-1);
	
    int rv = 0;
	
    cf_queue *queues[3];
    queues[0] = priority_q->high_q;
    queues[1] = priority_q->medium_q;
    queues[2] = priority_q->low_q;
	
    cf_queue *q;
    int found_index = -1;
	
    for (int q_itr = 0; q_itr < 3; q_itr++)
    {
        q = queues[q_itr];
		
        if (CF_Q_SZ(q)) {
			
            // it would be faster to have a local variable to hold the index,
            // and do it in bytes or something, but a delete
            // will change the read and write offset, so this is simpler for now
            // can optimize if necessary later....
			
            for (uint i = q->read_offset ;
				 i < q->write_offset ;
				 i++)
            {
				
                rv = cb(CF_Q_ELEM_PTR(q, i), udata);
				
                // rv == 0 is normal case, just increment to next point
                if (rv == -1) {
                    found_index = i;
                    break; // found what it was looking for, so break
                }
                else if (rv == -2) {
                    // found new candidate, but keep looking for one better
                    found_index = i;
                }
				
            };
			
            break; // only traverse the highest priority q
        }
    }
	
    if (found_index >= 0) {
        // found an element, so memcpy to buf, delete from q, and return
        memcpy(buf, CF_Q_ELEM_PTR(q, found_index), q->elementsz);
        cf_queue_delete_offset(q, found_index);
    }
	
    if (priority_q->threadsafe && (0 != pthread_mutex_unlock(&priority_q->LOCK))) {
        return(-1);
    }
	
    if (found_index == -1)
        return(CF_QUEUE_NOMATCH);
	
    return(0);
}
