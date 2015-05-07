/*
 * Copyright 2008-2015 Aerospike, Inc.
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
#include <aerospike/as_buffer_pool.h>
#include <citrusleaf/alloc.h>
#include <limits.h>

/******************************************************************************
 * Functions
 *****************************************************************************/

int
as_buffer_pool_init(as_buffer_pool* pool, uint32_t buffer_size, uint32_t request_max)
{
	// Initialize empty queue.
	pool->queue = cf_queue_create(sizeof(as_buffer_builder), true);
	pool->buffer_size = buffer_size;
	pool->request_max = (request_max > 0)? request_max : UINT_MAX;
	return (pool->queue)? 0 : -1;
}

int
as_buffer_pool_pop(as_buffer_pool* pool, as_buffer_builder* bb, uint32_t size)
{
	if (size > pool->buffer_size) {
		// Requested size is greater buffer sizes in pool.
		if (size > pool->request_max) {
			// Requested size is out of bounds.
			return -1;
		}

		// Allocate new buffer, but don't put back into pool.
		bb->data = cf_malloc(size);
		
		if (! bb->data) {
			return -2;
		}

		bb->capacity = size;
		bb->size = 0;
		return 0;
	}

	// Pop existing buffer from queue.
	int rc = cf_queue_pop(pool->queue, bb, CF_QUEUE_NOWAIT);

	if (rc == CF_QUEUE_OK) {
		bb->size = 0;
		return 0;
	}
	
	if (rc == CF_QUEUE_EMPTY) {
		// Queue is empty.  Create new buffer.  Queue can grow indefinitely.
		bb->data = cf_malloc(pool->buffer_size);
		
		if (! bb->data) {
			return -2;
		}
		
		bb->capacity = pool->buffer_size;
		bb->size = 0;
		return 0;
	}
	// Queue failure.
	return -3;
}

int
as_buffer_pool_push(as_buffer_pool* pool, as_buffer_builder* bb)
{
	if (bb->capacity <= pool->buffer_size) {
		// Put buffer back into pool.
		return cf_queue_push(pool->queue, bb);
	}
	else {
		// Do not put large buffers back into pool.
		cf_free(bb->data);
		bb->data = 0;
		return 0;
	}
}

int
as_buffer_pool_trim(as_buffer_pool* pool, int queue_count)
{
	// Number of buffers in pool may grow too large if burst of concurrent buffer usage happens.
	// Reduce free buffer queue count to a more acceptable level.
	int queue_size = cf_queue_sz(pool->queue);
	int delete_count = queue_size - queue_count;
	as_buffer_builder bb;
	int count = 0;

	while (count < delete_count) {
		if (cf_queue_pop(pool->queue, &bb, CF_QUEUE_NOWAIT) == CF_QUEUE_OK) {
			cf_free(bb.data);
			count++;
		}
		else {
			break;
		}
	}
	return count;
}

void
as_buffer_pool_destroy(as_buffer_pool* pool)
{
	// Empty and destroy queue.
	as_buffer_builder bb;
	while (cf_queue_pop(pool->queue, &bb, CF_QUEUE_NOWAIT) == CF_QUEUE_OK) {
		cf_free(bb.data);
	}
	cf_queue_destroy(pool->queue);
}
