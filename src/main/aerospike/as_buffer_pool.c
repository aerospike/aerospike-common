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
#include <aerospike/as_buffer_pool.h>
#include <citrusleaf/alloc.h>
#include <limits.h>

/******************************************************************************
 * Functions
 *****************************************************************************/

int
as_buffer_pool_init(as_buffer_pool* pool, uint32_t header_size, uint32_t buffer_size)
{
	// Initialize empty queue.
	pool->queue = cf_queue_create(sizeof(void*), true);
	pool->header_size = header_size;
	pool->buffer_size = buffer_size;
	return (pool->queue)? 0 : -1;
}

int
as_buffer_pool_pop(as_buffer_pool* pool, uint32_t size, as_buffer_result* buffer)
{
	size += pool->header_size;
	
	if (size > pool->buffer_size) {
		// Requested size is greater than fixed buffer size.
		// Allocate new buffer, but don't put back into pool.
		buffer->data = cf_malloc(size);
		
		if (! buffer->data) {
			return -1;
		}
		
		buffer->capacity = size - pool->header_size;
		return 2;
	}
	
	// Pop existing buffer from queue.
	int rc = cf_queue_pop(pool->queue, &buffer->data, CF_QUEUE_NOWAIT);
	
	if (rc == CF_QUEUE_OK) {
		buffer->capacity = pool->buffer_size - pool->header_size;
		return 0;
	}
	
	if (rc == CF_QUEUE_EMPTY) {
		// Queue is empty.  Create new buffer.  Queue can grow indefinitely.
		buffer->data = cf_malloc(pool->buffer_size);
		
		if (! buffer->data) {
			return -1;
		}
		
		buffer->capacity = pool->buffer_size - pool->header_size;
		return 1;
	}
	// Queue failure.
	return -2;
}

int
as_buffer_pool_push(as_buffer_pool* pool, void* buffer, uint32_t capacity)
{
	if (capacity + pool->header_size <= pool->buffer_size) {
		// Put buffer back into pool.
		if (cf_queue_push(pool->queue, &buffer) == CF_QUEUE_OK) {
			return 0;
		}
		else {
			cf_free(buffer);
			return -1;
		}
	}
	else {
		// Do not put large buffers back into pool.
		cf_free(buffer);
		return -2;
	}
}

int
as_buffer_pool_push_limit(as_buffer_pool* pool, void* buffer, uint32_t capacity, uint32_t max_buffers)
{
	if (capacity + pool->header_size <= pool->buffer_size) {
		// Put buffer back into pool up to a limit.
		if (cf_queue_push_limit(pool->queue, &buffer, max_buffers)) {
			// Success.
			return 0;
		}
		else {
			cf_free(buffer);
			return -1;
		}
	}
	else {
		// Do not put large buffers back into pool.
		cf_free(buffer);
		return -2;
	}
}

int
as_buffer_pool_drop_buffers(as_buffer_pool* pool, int buffer_count)
{
	// Number of buffers in pool may grow too large if burst of concurrent buffer usage happens.
	// Delete buffer_count buffers from pool.
	void* buffer;
	int count = 0;
	
	while (count < buffer_count) {
		if (cf_queue_pop(pool->queue, &buffer, CF_QUEUE_NOWAIT) == CF_QUEUE_OK) {
			cf_free(buffer);
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
	void* buffer;
	while (cf_queue_pop(pool->queue, &buffer, CF_QUEUE_NOWAIT) == CF_QUEUE_OK) {
		cf_free(buffer);
	}
	cf_queue_destroy(pool->queue);
}
