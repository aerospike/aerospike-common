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
#include <aerospike/as_queue.h>
#include <citrusleaf/alloc.h>
#include <string.h>

#define ITEMS_ON_HEAP 1
#define ALL_ON_HEAP 2

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

/**
 * We have to guard against wrap-around, so call this occasionally. We really
 * expect this will never get called, however it can be a symptom of a queue
 * getting really, really deep.
 */
static inline void
as_queue_unwrap(as_queue* queue)
{
	if ((queue->tail & 0xC0000000) != 0) {
		uint32_t sz = as_queue_size(queue);
		queue->head %= queue->capacity;
		queue->tail = queue->head + sz;
	}
}

static bool
as_queue_copy(as_queue* queue, uint32_t new_capacity, bool free_old)
{
	uint8_t* tmp = cf_malloc(new_capacity * queue->item_size);
	
	if (! tmp) {
		return false;
	}
	
	// end_sz is used bytes in old queue from insert point to end.
	size_t end_sz = (queue->capacity - (queue->head % queue->capacity)) * queue->item_size;
	
	memcpy(tmp, as_queue_get(queue, queue->head), end_sz);
	memcpy(&tmp[end_sz], queue->data, (queue->capacity * queue->item_size) - end_sz);
	
	if (free_old) {
		cf_free(queue->data);
	}
	queue->data = tmp;
	queue->head = 0;
	queue->tail = queue->capacity;
	queue->capacity = new_capacity;
	return true;
}

static bool
as_queue_increase_capacity(as_queue* queue)
{
	uint32_t new_capacity = queue->capacity * 2;
	
	if (queue->flags & ITEMS_ON_HEAP) {
		// Data already allocated on heap.
		// Check for The rare case where the queue is not fragmented, and realloc makes sense
		// and none of the offsets need to move.
		if (queue->head % queue->capacity == 0) {
			queue->data = cf_realloc(queue->data, new_capacity * queue->item_size);
			
			if (! queue->data) {
				return false;
			}
			queue->head = 0;
			queue->tail = queue->capacity;
			queue->capacity = new_capacity;
			return true;
		}
		
		// Copy to new queue.
		return as_queue_copy(queue, new_capacity, true);
	}
	
	// Switch over from stack to heap allocated.
	if (! as_queue_copy(queue, new_capacity, false)) {
		return false;
	}
	queue->flags |= ITEMS_ON_HEAP;
	return true;
}

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

bool
as_queue_init(as_queue* queue, uint32_t item_size, uint32_t capacity)
{
	queue->data = cf_malloc(capacity * item_size);
	
	if (! queue->data) {
		return false;
	}
	queue->capacity = capacity;
	queue->head = queue->tail = 0;
	queue->item_size = item_size;
	queue->total = 0;
	queue->flags = ITEMS_ON_HEAP;
	return true;
}

as_queue*
as_queue_create(uint32_t item_size, uint32_t capacity)
{
	as_queue* queue = (as_queue*)cf_malloc(sizeof(as_queue));
	
	if (! queue) {
		return 0;
	}

	if (! as_queue_init(queue, item_size, capacity)) {
		cf_free(queue);
		return 0;
	}
	queue->flags = ITEMS_ON_HEAP | ALL_ON_HEAP;
	return queue;
}

void
as_queue_destroy(as_queue* queue)
{
	if (queue->flags & ITEMS_ON_HEAP) {
		cf_free(queue->data);
		
		if (queue->flags & ALL_ON_HEAP) {
			cf_free(queue);
		}
	}
}

bool
as_queue_push(as_queue* queue, const void* ptr)
{
	// Check queue length.
	if (as_queue_size(queue) == queue->capacity) {
		if (! as_queue_increase_capacity(queue)) {
			return false;
		}
	}

	memcpy(as_queue_get(queue, queue->tail), ptr, queue->item_size);
	queue->tail++;
	as_queue_unwrap(queue);
	return true;
}

bool
as_queue_push_limit(as_queue* queue, const void* ptr)
{
	if (as_queue_size(queue) >= queue->capacity) {
		return false;
	}

	memcpy(as_queue_get(queue, queue->tail), ptr, queue->item_size);
	queue->tail++;
	as_queue_unwrap(queue);
	return true;
}

bool
as_queue_push_head(as_queue *queue, const void* ptr)
{
	if (as_queue_size(queue) == queue->capacity) {
		if (! as_queue_increase_capacity(queue)) {
			return false;
		}
	}

	if (queue->head == 0) {
		queue->head += queue->capacity;
		queue->tail += queue->capacity;
	}

	queue->head--;
	memcpy(as_queue_get(queue, queue->head), ptr, queue->item_size);
	as_queue_unwrap(queue);
	return true;
}

bool
as_queue_push_head_limit(as_queue *queue, const void* ptr)
{
	if (as_queue_size(queue) >= queue->capacity) {
		return false;
	}

	if (queue->head == 0) {
		queue->head += queue->capacity;
		queue->tail += queue->capacity;
	}

	queue->head--;
	memcpy(as_queue_get(queue, queue->head), ptr, queue->item_size);
	as_queue_unwrap(queue);
	return true;
}
