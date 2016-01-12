/*
 * Copyright 2008-2016 Aerospike, Inc.
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
#include <citrusleaf/cf_queue_priority.h>
#include <citrusleaf/cf_clock.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_queue_priority *cf_queue_priority_create(size_t element_sz, bool threadsafe)
{
	cf_queue_priority *q = (cf_queue_priority*)cf_malloc(sizeof(cf_queue_priority));

	if (! q) {
		return NULL;
	}

	q->threadsafe = threadsafe;

	if (! (q->low_q = cf_queue_create(element_sz, false))) {
		goto Fail1;
	}

	if (! (q->medium_q = cf_queue_create(element_sz, false))) {
		goto Fail2;
	}

	if (! (q->high_q = cf_queue_create(element_sz, false))) {
		goto Fail3;
	}

	if (! threadsafe) {
		return q;
	}

	if (0 != pthread_mutex_init(&q->LOCK, NULL)) {
		goto Fail4;
	}

	if (0 != pthread_cond_init(&q->CV, NULL)) {
		goto Fail5;
	}

	return q;

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

	return NULL;
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

static inline void cf_queue_priority_lock(cf_queue_priority *q)
{
	if (q->threadsafe) {
		pthread_mutex_lock(&q->LOCK);
	}
}

static inline void cf_queue_priority_unlock(cf_queue_priority *q)
{
	if (q->threadsafe) {
		pthread_mutex_unlock(&q->LOCK);
	}
}

int cf_queue_priority_push(cf_queue_priority *q, const void *ptr, int pri)
{
	cf_queue_priority_lock(q);

	int rv = CF_QUEUE_ERR;

	if (pri == CF_QUEUE_PRIORITY_HIGH) {
		rv = cf_queue_push(q->high_q, ptr);
	}
	else if (pri == CF_QUEUE_PRIORITY_MEDIUM) {
		rv = cf_queue_push(q->medium_q, ptr);
	}
	else if (pri == CF_QUEUE_PRIORITY_LOW) {
		rv = cf_queue_push(q->low_q, ptr);
	}

	if (rv == 0 && q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	cf_queue_priority_unlock(q);
	return rv;
}

int cf_queue_priority_pop(cf_queue_priority *q, void *buf, int ms_wait)
{
	cf_queue_priority_lock(q);

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
				return CF_QUEUE_EMPTY;
			}
			else {
				pthread_cond_timedwait(&q->CV, &q->LOCK, &tp);

				if (CF_Q_PRI_EMPTY(q)) {
					pthread_mutex_unlock(&q->LOCK);
					return CF_QUEUE_EMPTY;
				}
			}
		}
	}

	int rv = CF_QUEUE_EMPTY;

	if (CF_Q_SZ(q->high_q)) {
		rv = cf_queue_pop(q->high_q, buf, 0);
	}
	else if (CF_Q_SZ(q->medium_q)) {
		rv = cf_queue_pop(q->medium_q, buf, 0);
	}
	else if (CF_Q_SZ(q->low_q)) {
		rv = cf_queue_pop(q->low_q, buf, 0);
	}

	cf_queue_priority_unlock(q);
	return rv;
}

int cf_queue_priority_sz(cf_queue_priority *q)
{
	int rv = 0;

	cf_queue_priority_lock(q);
	rv += cf_queue_sz(q->high_q);
	rv += cf_queue_sz(q->medium_q);
	rv += cf_queue_sz(q->low_q);
	cf_queue_priority_unlock(q);

	return rv;
}

/**
 * Use this function to find an element to pop from the queue using a reduce
 * callback function. Have the callback function return -1 when you know you
 * want to pop the element immediately, return -2 when the element is the best
 * candidate for popping found so far but you want to keep looking, and return 0
 * when you are not interested in popping the element. You then pop the best
 * candidate you've found - either the "-1 case" or the last "-2 case". If you
 * have not found a suitable candidate, CF_QUEUE_NOMATCH is returned.
 */
int cf_queue_priority_reduce_pop(cf_queue_priority *priority_q, void *buf, cf_queue_reduce_fn cb, void *udata)
{
	cf_queue_priority_lock(priority_q);

	cf_queue *queues[3];

	queues[0] = priority_q->high_q;
	queues[1] = priority_q->medium_q;
	queues[2] = priority_q->low_q;

	cf_queue *q;
	int found_index = -1;

	for (int q_itr = 0; q_itr < 3; q_itr++) {
		q = queues[q_itr];

		if (CF_Q_SZ(q) == 0) {
			continue;
		}

		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);

			if (rv == 0) {
				continue;
			}

			if (rv == -1) {
				// Found what it was looking for, so break.
				found_index = i;
				break;
			}

			if (rv == -2) {
				// Found new candidate, but keep looking for a better one.
				found_index = i;
			}
		}

		// Only traverse the highest priority q with elements.
		// TODO - is this the right thing to do ???
		break;
	}

	if (found_index >= 0) {
		// Found an element, so copy to 'buf', delete from q, and return.
		memcpy(buf, CF_Q_ELEM_PTR(q, found_index), q->element_sz);
		cf_queue_delete_offset(q, found_index);
	}

	cf_queue_priority_unlock(priority_q);
	return found_index == -1 ? CF_QUEUE_NOMATCH : CF_QUEUE_OK;
}

//
// This assumes the element we're looking for is unique! Returns
// CF_QUEUE_NOMATCH if the element is not found or not moved.
//
int cf_queue_priority_change(cf_queue_priority *priority_q, const void *ptr, int new_pri)
{
	cf_queue_priority_lock(priority_q);

	cf_queue *queues[3];

	queues[0] = priority_q->high_q;
	queues[1] = priority_q->medium_q;
	queues[2] = priority_q->low_q;

	int dest_q_itr = CF_QUEUE_PRIORITY_HIGH - new_pri;
	cf_queue *q;

	for (int q_itr = 0; q_itr < 3; q_itr++) {
		q = queues[q_itr];

		if (q_itr == dest_q_itr || CF_Q_SZ(q) == 0) {
			continue;
		}

		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			if (memcmp(CF_Q_ELEM_PTR(q, i), ptr, q->element_sz) == 0) {
				// Move it to the queue with desired priority.
				cf_queue_delete_offset(q, i);
				cf_queue_push(queues[dest_q_itr], ptr);

				cf_queue_priority_unlock(priority_q);
				return CF_QUEUE_OK;
			}
		}
	}

	cf_queue_priority_unlock(priority_q);
	return CF_QUEUE_NOMATCH;
}

//
// Reduce the inner queues whose priorities are different to 'new_pri'. If the
// callback returns -1, move that element to the inner queue whose priority is
// 'new_pri' and return CF_QUEUE_OK. Returns CF_QUEUE_NOMATCH if callback never
// triggers a move.
//
int cf_queue_priority_reduce_change(cf_queue_priority *priority_q, int new_pri, cf_queue_reduce_fn cb, void *udata)
{
	cf_queue_priority_lock(priority_q);

	cf_queue *queues[3];

	queues[0] = priority_q->high_q;
	queues[1] = priority_q->medium_q;
	queues[2] = priority_q->low_q;

	int dest_q_itr = CF_QUEUE_PRIORITY_HIGH - new_pri;
	cf_queue *q;

	for (int q_itr = 0; q_itr < 3; q_itr++) {
		q = queues[q_itr];

		if (q_itr == dest_q_itr || CF_Q_SZ(q) == 0) {
			continue;
		}

		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);

			if (rv == 0) {
				continue;
			}

			if (rv == -1) {
				// Found it - move to desired queue and return.
				byte buf[q->element_sz];

				memcpy(buf, CF_Q_ELEM_PTR(q, i), q->element_sz);
				cf_queue_delete_offset(q, i);
				cf_queue_push(queues[dest_q_itr], buf);

				cf_queue_priority_unlock(priority_q);
				return CF_QUEUE_OK;
			}
		}
	}

	cf_queue_priority_unlock(priority_q);
	return CF_QUEUE_NOMATCH;
}
