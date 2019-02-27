/*
 * Copyright 2008-2019 Aerospike, Inc.
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
#include <aerospike/as_queue_mt.h>
#include <citrusleaf/cf_clock.h>
#include <citrusleaf/alloc.h>

#define ITEMS_ON_HEAP 1
#define ALL_ON_HEAP 2

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static void
as_queue_mt_wait(as_queue_mt* queue, int wait_ms)
{
	// as_queue_empty() is checked in as_queue_pop(), so no need
	// to check here on AS_QUEUE_NOWAIT.
	if (wait_ms == AS_QUEUE_NOWAIT || ! as_queue_empty(&queue->queue)) {
		return;
	}

	if (wait_ms == AS_QUEUE_FOREVER) {
		// Note that we have to use a while() loop. The pthread_cond_signal()
		// documentation says that AT LEAST ONE waiting thread will be awakened.
		// If more than one are awakened, the first will get the popped element,
		// others will find the queue empty and go back to waiting.
		do {
			pthread_cond_wait(&queue->cond, &queue->lock);
		} while (as_queue_empty(&queue->queue));

		return;
	}

	struct timespec tp;
	cf_set_wait_timespec(wait_ms, &tp);
	pthread_cond_timedwait(&queue->cond, &queue->lock, &tp);
}

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

bool
as_queue_mt_init(as_queue_mt* queue, uint32_t item_size, uint32_t capacity)
{
	if (! as_queue_init(&queue->queue, item_size, capacity)) {
		return false;
	}

	if (pthread_mutex_init(&queue->lock, NULL) != 0) {
		as_queue_destroy(&queue->queue);
		return false;
	}

	if (pthread_cond_init(&queue->cond, NULL) != 0) {
		pthread_mutex_destroy(&queue->lock);
		as_queue_destroy(&queue->queue);
		return false;
	}
	return true;
}

as_queue_mt*
as_queue_mt_create(uint32_t item_size, uint32_t capacity)
{
	as_queue_mt* queue = cf_malloc(sizeof(as_queue_mt));

	if (! queue) {
		return NULL;
	}

	if (! as_queue_mt_init(queue, item_size, capacity)) {
		cf_free(queue);
		return NULL;
	}
	queue->queue.flags = ITEMS_ON_HEAP | ALL_ON_HEAP;
	return queue;
}

bool
as_queue_mt_pop(as_queue_mt* queue, void* ptr, int wait_ms)
{
	pthread_mutex_lock(&queue->lock);
	as_queue_mt_wait(queue, wait_ms);
	bool status = as_queue_pop(&queue->queue, ptr);
	pthread_mutex_unlock(&queue->lock);
	return status;
}

bool
as_queue_mt_pop_tail(as_queue_mt* queue, void* ptr, int wait_ms)
{
	pthread_mutex_lock(&queue->lock);
	as_queue_mt_wait(queue, wait_ms);
	bool status = as_queue_pop_tail(&queue->queue, ptr);
	pthread_mutex_unlock(&queue->lock);
	return status;
}
