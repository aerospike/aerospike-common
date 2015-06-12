/*
 * Copyright 2008-2014 Aerospike, Inc.
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

cf_queue *cf_queue_create(size_t elementsz, bool threadsafe)
{
	cf_queue *q = (cf_queue*)cf_malloc(sizeof(cf_queue));

	if (! q) {
		return NULL;
	}

	q->allocsz = CF_QUEUE_ALLOCSZ;
	q->write_offset = q->read_offset = 0;
	q->elementsz = elementsz;
	q->threadsafe = threadsafe;

	q->queue = (uint8_t*)cf_malloc(CF_QUEUE_ALLOCSZ * elementsz);

	if (! q->queue) {
		cf_free(q);
		return NULL;
	}

	if (! q->threadsafe) {
		return q;
	}

	if (0 != pthread_mutex_init(&q->LOCK, NULL)) {
		cf_free(q->queue);
		cf_free(q);
		return NULL;
	}

	if (0 != pthread_cond_init(&q->CV, NULL)) {
		pthread_mutex_destroy(&q->LOCK);
		cf_free(q->queue);
		cf_free(q);
		return NULL;
	}

	return q;
}

void cf_queue_destroy(cf_queue *q)
{
	if (q->threadsafe) {
		pthread_cond_destroy(&q->CV);
		pthread_mutex_destroy(&q->LOCK);
	}

	memset(q->queue, 0, sizeof(q->allocsz * q->elementsz));
	cf_free(q->queue);
	memset(q, 0, sizeof(cf_queue));
	cf_free(q);
}

int cf_queue_sz(cf_queue *q)
{
	int rv;

	if (q->threadsafe) {
		pthread_mutex_lock(&q->LOCK);
	}

	rv = CF_Q_SZ(q);

	if (q->threadsafe) {
		pthread_mutex_unlock(&q->LOCK);
	}

	return rv;
}

//
// Internal function. Call with new size with lock held. This function only
// works on full queues.
//
static int cf_queue_resize(cf_queue *q, uint32_t new_sz)
{
	// Check if queue is not full.
	if (CF_Q_SZ(q) != q->allocsz) {
		return -1;
	}

	// The rare case where the queue is not fragmented, and realloc makes sense
	// and none of the offsets need to move.
	if (0 == q->read_offset % q->allocsz) {
		q->queue = (uint8_t*)cf_realloc(q->queue, new_sz * q->elementsz);

		if (! q->queue) {
			return -1;
		}

		q->read_offset = 0;
		q->write_offset = q->allocsz;
	}
	else {
		byte *newq = (uint8_t*)cf_malloc(new_sz * q->elementsz);

		if (! newq) {
			return -1;
		}

		// endsz is used bytes in old queue from insert point to end.
		size_t endsz = (q->allocsz - (q->read_offset % q->allocsz)) * q->elementsz;

		memcpy(&newq[0], CF_Q_ELEM_PTR(q, q->read_offset), endsz);
		memcpy(&newq[endsz], &q->queue[0], (q->allocsz * q->elementsz) - endsz);

		cf_free(q->queue);
		q->queue = newq;

		q->write_offset = q->allocsz;
		q->read_offset = 0;
	}

	q->allocsz = new_sz;

	return 0;
}

//
// We have to guard against wrap-around, call this occasionally. We really
// expect this will never get called, however it can be a symptom of a queue
// getting really, really deep.
//
static void cf_queue_unwrap(cf_queue *q)
{
	int sz = CF_Q_SZ(q);

	q->read_offset %= q->allocsz;
	q->write_offset = q->read_offset + sz;
}

int cf_queue_push(cf_queue *q, void *ptr)
{
	if (! q || ! ptr) {
		return -1;
	}

	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return -1;
	}

	// Check queue length.
	if (CF_Q_SZ(q) == q->allocsz) {
		if (0 != cf_queue_resize(q, q->allocsz * 2)) {
			if (q->threadsafe) {
				pthread_mutex_unlock(&q->LOCK);
			}

			return -1;
		}
	}

	// TODO - if queues are power of 2, this can be a shift.
	memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->elementsz);
	q->write_offset++;

	// We're at risk of overflow if the write offset is that high.
	if (q->write_offset & 0xC0000000) {
		cf_queue_unwrap(q);
	}

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	return 0;
}

//
// Push element on the queue only if size < limit.
//
bool cf_queue_push_limit(cf_queue *q, void *ptr, uint32_t limit)
{
	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return false;
	}

	uint32_t size = CF_Q_SZ(q);

	if (size >= limit) {
		if (q->threadsafe) {
			pthread_mutex_unlock(&q->LOCK);
		}

		return false;
	}

	if (size == q->allocsz) {
		if (0 != cf_queue_resize(q, q->allocsz * 2)) {
			if (q->threadsafe) {
				pthread_mutex_unlock(&q->LOCK);
			}

			return false;
		}
	}

	// TODO - if queues are power of 2, this can be a shift.
	memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->elementsz);
	q->write_offset++;

	// We're at risk of overflow if the write offset is that high.
	if (q->write_offset & 0xC0000000) {
		cf_queue_unwrap(q);
	}

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return false;
	}

	return true;
}

//
// Same as cf_queue_push() except it's a no-op if element is already queued.
//
int cf_queue_push_unique(cf_queue *q, void *ptr)
{
	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return -1;
	}

	// Check if element is already queued.
	if (CF_Q_SZ(q)) {
		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			if (0 == memcmp(CF_Q_ELEM_PTR(q, i), ptr, q->elementsz)) {
				if (q->threadsafe) {
					pthread_mutex_unlock(&q->LOCK);
				}
				// Element is already queued.
				// TODO - return 0 if all callers regard this as normal?
				return -2;
			}
		}
	}

	if (CF_Q_SZ(q) == q->allocsz) {
		if (0 != cf_queue_resize(q, q->allocsz * 2)) {
			if (q->threadsafe) {
				pthread_mutex_unlock(&q->LOCK);
			}

			return -1;
		}
	}

	// TODO - if queues are power of 2, this can be a shift.
	memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->elementsz);
	q->write_offset++;

	// We're at risk of overflow if the write offset is that high.
	if (q->write_offset & 0xC0000000) {
		cf_queue_unwrap(q);
	}

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	return 0;
}

//
// Push to the front of the queue.
//
int cf_queue_push_head(cf_queue *q, void *ptr)
{
	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return -1;
	}

	if (CF_Q_SZ(q) == q->allocsz) {
		if (0 != cf_queue_resize(q, q->allocsz * 2)) {
			if (q->threadsafe) {
				pthread_mutex_unlock(&q->LOCK);
			}

			return -1;
		}
	}

	// Easy case, tail insert is head insert.
	if (q->read_offset == q->write_offset) {
		memcpy(CF_Q_ELEM_PTR(q, q->write_offset), ptr, q->elementsz);
		q->write_offset++;
	}
	// Another easy case, there's space up front.
	else if (q->read_offset > 0) {
		q->read_offset--;
		memcpy(CF_Q_ELEM_PTR(q, q->read_offset), ptr, q->elementsz);
	}
	// Hard case, we're going to have to shift everything back.
	// TODO - we can do better than this...
	else {
		memmove(CF_Q_ELEM_PTR(q, 1), CF_Q_ELEM_PTR(q, 0), q->elementsz * CF_Q_SZ(q));
		memcpy(CF_Q_ELEM_PTR(q, 0), ptr, q->elementsz);
		q->write_offset++;
	}

	// We're at risk of overflow if the write offset is that high.
	if (q->write_offset & 0xC0000000) {
		cf_queue_unwrap(q);
	}

	if (q->threadsafe) {
		pthread_cond_signal(&q->CV);
	}

	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	return 0;
}

//
// If ms_wait < 0, wait forever.
// If ms_wait = 0, don't wait at all.
// If ms_wait > 0, wait that number of milliseconds.
//
int cf_queue_pop(cf_queue *q, void *buf, int ms_wait)
{
	if (! q) {
		return -1;
	}

	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return -1;
	}

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

	memcpy(buf, CF_Q_ELEM_PTR(q, q->read_offset), q->elementsz);
	q->read_offset++;

	// This probably keeps the cache fresher because the queue is fully empty.
	if (q->read_offset == q->write_offset) {
		q->read_offset = q->write_offset = 0;
	}

	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	return 0;
}

void cf_queue_delete_offset(cf_queue *q, uint32_t index)
{
	index %= q->allocsz;

	uint32_t r_index = q->read_offset % q->allocsz;
	uint32_t w_index = q->write_offset % q->allocsz;

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
		memmove(&q->queue[(r_index + 1) * q->elementsz],
				&q->queue[r_index * q->elementsz],
				(index - r_index) * q->elementsz);
		q->read_offset++;
		return;
	}

	if (index < w_index) {
		// The memory copy is overlapping, so must use memmove().
		memmove(&q->queue[index * q->elementsz],
				&q->queue[(index + 1) * q->elementsz],
				(w_index - index - 1) * q->elementsz);
		q->write_offset--;
	}
}

//
// Iterate over all queue members, calling the callback.
//
int cf_queue_reduce(cf_queue *q,  cf_queue_reduce_fn cb, void *udata)
{
	if (! q) {
		return -1;
	}

	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return -1;
	}

	if (CF_Q_SZ(q)) {
		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);

			// rv == 0 is normal case, just increment to next element.

			if (rv == -1) {
				// Found what it was looking for, stop reducing.
				break;
			}
			else if (rv == -2) {
				// Delete, and stop reducing.
				cf_queue_delete_offset(q, i);
				goto Found;
			}
		}
	}

Found:
	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	return 0;
}


//
// Iterate over all queue members starting from the tail, calling the callback.
//
int cf_queue_reduce_reverse(cf_queue *q, cf_queue_reduce_fn cb, void *udata)
{
	if (! q) {
		return -1;
	}

	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return -1;
	}

	if (CF_Q_SZ(q)) {
		for (uint32_t i = q->write_offset - 1; i >= q->read_offset; i--) {
			int rv = cb(CF_Q_ELEM_PTR(q, i), udata);

			// rv == 0 is normal case, just increment to next element.

			if (rv == -1) {
				// Found what it was looking for, stop reducing.
				break;
			}
			else if (rv == -2) {
				// Delete, and stop reducing.
				cf_queue_delete_offset(q, i);
				goto Found;
			}
		}
	}

Found:
	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	return 0;
}

//
// Special case: delete elements from the queue. Pass 'true' as the 'only_one'
// parameter if there can be only one element with this value on the queue.
//
int cf_queue_delete(cf_queue *q, void *buf, bool only_one)
{
	if (! q) {
		return CF_QUEUE_ERR;
	}

	if (q->threadsafe && (0 != pthread_mutex_lock(&q->LOCK))) {
		return CF_QUEUE_ERR;
	}

	bool found = false;

	if (CF_Q_SZ(q)) {
		for (uint32_t i = q->read_offset; i < q->write_offset; i++) {
			int rv = 0;

			// If buf is null, rv is always 0 and we delete all elements.
			if (buf) {
				rv = memcmp(CF_Q_ELEM_PTR(q, i), buf, q->elementsz);
			}

			if (rv == 0) {
				cf_queue_delete_offset(q, i);
				found = true;

				if (only_one) {
					goto Done;
				}
			}
		}
	}

Done:
	if (q->threadsafe && (0 != pthread_mutex_unlock(&q->LOCK))) {
		return -1;
	}

	if (! found) {
		return CF_QUEUE_EMPTY;
	}
	else {
		return CF_QUEUE_OK;
	}
}

int cf_queue_delete_all(cf_queue *q) {
	return cf_queue_delete(q, NULL, false);
}
