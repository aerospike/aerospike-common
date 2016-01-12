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
#include <citrusleaf/cf_queue.h>
#include <citrusleaf/cf_clock.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

bool
cf_queue_init(cf_queue* q, size_t element_sz, uint32_t capacity, bool threadsafe)
{
	q->alloc_sz = capacity;
	q->write_offset = q->read_offset = 0;
	q->element_sz = element_sz;
	q->threadsafe = threadsafe;
	q->free_struct = false;
	
	q->elements = (uint8_t*)cf_malloc(capacity * element_sz);
	
	if (! q->elements) {
		return false;
	}
	
	if (! q->threadsafe) {
		return q;
	}
	
	if (0 != pthread_mutex_init(&q->LOCK, NULL)) {
		cf_free(q->elements);
		return false;
	}
	
	if (0 != pthread_cond_init(&q->CV, NULL)) {
		pthread_mutex_destroy(&q->LOCK);
		cf_free(q->elements);
		return false;
	}
	return true;
}

cf_queue *cf_queue_create(size_t element_sz, bool threadsafe)
{
	cf_queue *q = (cf_queue*)cf_malloc(sizeof(cf_queue));

	if (! q) {
		return NULL;
	}

	if (! cf_queue_init(q, element_sz, CF_QUEUE_ALLOCSZ, threadsafe)) {
		cf_free(q);
		return NULL;
	}
	q->free_struct = true;
	return q;
}

void cf_queue_destroy(cf_queue *q)
{
	if (q->threadsafe) {
		pthread_cond_destroy(&q->CV);
		pthread_mutex_destroy(&q->LOCK);
	}

	memset(q->elements, 0, sizeof(q->alloc_sz * q->element_sz));
	cf_free(q->elements);
	
	if (q->free_struct) {
		memset(q, 0, sizeof(cf_queue));
		cf_free(q);
	}
}

static inline void cf_queue_lock(cf_queue *q)
{
	if (q->threadsafe) {
		pthread_mutex_lock(&q->LOCK);
	}
}

static inline void cf_queue_unlock(cf_queue *q)
{
	if (q->threadsafe) {
		pthread_mutex_unlock(&q->LOCK);
	}
}

int cf_queue_sz(cf_queue *q)
{
	cf_queue_lock(q);
	int rv = CF_Q_SZ(q);
	cf_queue_unlock(q);

	return rv;
}

//
// Internal function. Call with new size with lock held. This function only
// works on full queues.
//
static int cf_queue_resize(cf_queue *q, uint32_t new_sz)
{
	// Check if queue is not full.
	if (CF_Q_SZ(q) != q->alloc_sz) {
		return CF_QUEUE_ERR;
	}

	// The rare case where the queue is not fragmented, and realloc makes sense
	// and none of the offsets need to move.
	if (0 == q->read_offset % q->alloc_sz) {
		q->elements = (uint8_t*)cf_realloc(q->elements, new_sz * q->element_sz);

		if (! q->elements) {
			return CF_QUEUE_ERR;
		}

		q->read_offset = 0;
		q->write_offset = q->alloc_sz;
	}
	else {
		byte *newq = (uint8_t*)cf_malloc(new_sz * q->element_sz);

		if (! newq) {
			return CF_QUEUE_ERR;
		}

		// end_sz is used bytes in old queue from insert point to end.
		size_t end_sz = (q->alloc_sz - (q->read_offset % q->alloc_sz)) * q->element_sz;

		memcpy(&newq[0], CF_Q_ELEM_PTR(q, q->read_offset), end_sz);
		memcpy(&newq[end_sz], &q->elements[0], (q->alloc_sz * q->element_sz) - end_sz);

		cf_free(q->elements);
		q->elements = newq;

		q->write_offset = q->alloc_sz;
		q->read_offset = 0;
	}

	q->alloc_sz = new_sz;

	return CF_QUEUE_OK;
}

//
// We have to guard against wrap-around, call this occasionally. We really
// expect this will never get called, however it can be a symptom of a queue
// getting really, really deep.
//
static inline void cf_queue_unwrap(cf_queue *q)
{
	if ((q->write_offset & 0xC0000000) != 0) {
		int sz = CF_Q_SZ(q);

		q->read_offset %= q->alloc_sz;
		q->write_offset = q->read_offset + sz;
	}
}

int cf_queue_push(cf_queue *q, const void *ptr)
{
	cf_queue_lock(q);

	// Check queue length.
	if (CF_Q_SZ(q) == q->alloc_sz) {
		if (0 != cf_queue_resize(q, q->alloc_sz * 2)) {
			cf_queue_unlock(q);
			return CF_QUEUE_ERR;
		}
	}

	// TODO - if queues are power of 2, this can be a shift.
	memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->element_sz);
	q->write_offset++;
	cf_queue_unwrap(q);

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	cf_queue_unlock(q);
	return CF_QUEUE_OK;
}

//
// Push element on the queue only if size < limit.
//
bool cf_queue_push_limit(cf_queue *q, const void *ptr, uint32_t limit)
{
	cf_queue_lock(q);

	uint32_t size = CF_Q_SZ(q);

	if (size >= limit) {
		cf_queue_unlock(q);
		return false;
	}

	if (size == q->alloc_sz) {
		if (0 != cf_queue_resize(q, q->alloc_sz * 2)) {
			cf_queue_unlock(q);
			return false;
		}
	}

	// TODO - if queues are power of 2, this can be a shift.
	memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->element_sz);
	q->write_offset++;
	cf_queue_unwrap(q);

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	cf_queue_unlock(q);
	return true;
}

//
// Same as cf_queue_push() except it's a no-op if element is already queued.
//
int cf_queue_push_unique(cf_queue *q, const void *ptr)
{
	cf_queue_lock(q);

	// Check if element is already queued.
	if (CF_Q_SZ(q) != 0) {
		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			if (0 == memcmp(CF_Q_ELEM_PTR(q, i), ptr, q->element_sz)) {
				// Element is already queued.
				// TODO - return 0 if all callers regard this as normal?
				cf_queue_unlock(q);
				return -2;
			}
		}
	}

	if (CF_Q_SZ(q) == q->alloc_sz) {
		if (0 != cf_queue_resize(q, q->alloc_sz * 2)) {
			cf_queue_unlock(q);
			return CF_QUEUE_ERR;
		}
	}

	// TODO - if queues are power of 2, this can be a shift.
	memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->element_sz);
	q->write_offset++;
	cf_queue_unwrap(q);

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	cf_queue_unlock(q);
	return CF_QUEUE_OK;
}

//
// Push to the front of the queue.
//
int cf_queue_push_head(cf_queue *q, const void *ptr)
{
	cf_queue_lock(q);

	if (CF_Q_SZ(q) == q->alloc_sz) {
		if (0 != cf_queue_resize(q, q->alloc_sz * 2)) {
			cf_queue_unlock(q);
			return CF_QUEUE_ERR;
		}
	}

	// Easy case, tail insert is head insert.
	if (CF_Q_EMPTY(q)) {
		memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->element_sz);
		q->write_offset++;
	}
	// Another easy case, there's space up front.
	else if (q->read_offset > 0) {
		q->read_offset--;
		memcpy(CF_Q_ELEM_PTR(q, q->read_offset), ptr, q->element_sz);
	}
	// Hard case, we're going to have to shift everything back.
	// TODO - we can do better than this...
	else {
		memmove(CF_Q_ELEM_PTR(q, 1), CF_Q_ELEM_PTR(q, 0), q->element_sz * CF_Q_SZ(q));
		memcpy(CF_Q_ELEM_PTR(q, 0), ptr, q->element_sz);
		q->write_offset++;
	}

	cf_queue_unwrap(q);

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	cf_queue_unlock(q);
	return CF_QUEUE_OK;
}

//
// If ms_wait < 0, wait forever.
// If ms_wait = 0, don't wait at all.
// If ms_wait > 0, wait that number of milliseconds.
//
int cf_queue_pop(cf_queue *q, void *buf, int ms_wait)
{
	cf_queue_lock(q);

	struct timespec tp;

	if (ms_wait > 0) {
		cf_set_wait_timespec(ms_wait, &tp);
	}

	if (q->threadsafe) {

		// Note that we have to use a while() loop. The pthread_cond_signal()
		// documentation says that AT LEAST ONE waiting thread will be awakened.
		// If more than one are awakened, the first will get the popped element,
		// others will find the queue empty and go back to waiting.

		while (CF_Q_EMPTY(q)) {
			if (CF_QUEUE_FOREVER == ms_wait) {
				pthread_cond_wait(&q->CV, &q->LOCK);
			}
			else if (CF_QUEUE_NOWAIT == ms_wait) {
				pthread_mutex_unlock(&q->LOCK);
				return CF_QUEUE_EMPTY;
			}
			else {
				pthread_cond_timedwait(&q->CV, &q->LOCK, &tp);

				if (CF_Q_EMPTY(q)) {
					pthread_mutex_unlock(&q->LOCK);
					return CF_QUEUE_EMPTY;
				}
			}
		}
	}
	else if (CF_Q_EMPTY(q)) {
		return CF_QUEUE_EMPTY;
	}

	memcpy(buf, CF_Q_ELEM_PTR(q, q->read_offset), q->element_sz);
	q->read_offset++;

	// This probably keeps the cache fresher because the queue is fully empty.
	if (q->read_offset == q->write_offset) {
		q->read_offset = q->write_offset = 0;
	}

	cf_queue_unlock(q);
	return CF_QUEUE_OK;
}

void cf_queue_delete_offset(cf_queue *q, uint32_t index)
{
	index %= q->alloc_sz;

	uint32_t r_index = q->read_offset % q->alloc_sz;
	uint32_t w_index = q->write_offset % q->alloc_sz;

	// Assumes index is validated!

	// If we're deleting the one at the head, just increase the read offset.
	if (index == r_index) {
		q->read_offset++;
		return;
	}

	// If we're deleting the tail just decrease the write offset.
	if (w_index && (index == w_index - 1)) {
		q->write_offset--;
		return;
	}

	if (index > r_index) {
		// The memory copy is overlapping, so must use memmove().
		memmove(&q->elements[(r_index + 1) * q->element_sz],
				&q->elements[r_index * q->element_sz],
				(index - r_index) * q->element_sz);
		q->read_offset++;
		return;
	}

	if (index < w_index) {
		// The memory copy is overlapping, so must use memmove().
		memmove(&q->elements[index * q->element_sz],
				&q->elements[(index + 1) * q->element_sz],
				(w_index - index - 1) * q->element_sz);
		q->write_offset--;
	}
}

//
// Iterate over all queue members, calling the callback.
//
int cf_queue_reduce(cf_queue *q,  cf_queue_reduce_fn cb, void *udata)
{
	cf_queue_lock(q);

	if (CF_Q_SZ(q) != 0) {
		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);

			if (rv == 0) {
				continue;
			}

			if (rv == -1) {
				// Found what it was looking for, stop reducing.
				break;
			}

			if (rv == -2) {
				// Delete, and stop reducing.
				cf_queue_delete_offset(q, i);
				break;
			}
		}
	}

	cf_queue_unlock(q);
	return CF_QUEUE_OK;
}

//
// Iterate over all queue members starting from the tail, calling the callback.
//
int cf_queue_reduce_reverse(cf_queue *q, cf_queue_reduce_fn cb, void *udata)
{
	cf_queue_lock(q);

	if (CF_Q_SZ(q) != 0) {
		for (int i = (int)q->write_offset - 1; i >= (int)q->read_offset; i--) {
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);

			if (rv == 0) {
				continue;
			}

			if (rv == -1) {
				// Found what it was looking for, stop reducing.
				break;
			}

			if (rv == -2) {
				// Delete, and stop reducing.
				cf_queue_delete_offset(q, i);
				break;
			}
		}
	}

	cf_queue_unlock(q);
	return CF_QUEUE_OK;
}

//
// Delete element(s) from the queue. Pass 'only_one' as true if there can be
// only one element with this value on the queue.
//
int cf_queue_delete(cf_queue *q, const void *ptr, bool only_one)
{
	cf_queue_lock(q);

	bool found = false;

	if (CF_Q_SZ(q) != 0) {
		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			int rv = 0;

			// If buf is null, rv is always 0 and we delete all elements.
			if (ptr) {
				rv = memcmp(CF_Q_ELEM_PTR(q, i), ptr, q->element_sz);
			}

			if (rv == 0) {
				cf_queue_delete_offset(q, i);
				found = true;

				if (only_one) {
					break;
				}
			}
		}
	}

	cf_queue_unlock(q);
	return found ? CF_QUEUE_OK : CF_QUEUE_EMPTY;
}

int cf_queue_delete_all(cf_queue *q) {
	return cf_queue_delete(q, NULL, false);
}
