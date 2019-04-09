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
#pragma once

#include <string.h>

#include <aerospike/as_std.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * TYPES
 ******************************************************************************/

/**
 * A fast, non-thread-safe dynamic queue implementation.
 * as_queue is not part of the generic as_val family.
 */
typedef struct as_queue_s {
	/**
	 * The block of items in the queue.
	 */
	uint8_t* data;

	/**
	 * The total number of items allocated.
	 */
	uint32_t capacity;

	/**
	 * Item offset of head.
	 */
	uint32_t head;

	/**
	 * Item offset of tail.
	 */
	uint32_t tail;

	/**
	 * The size of a single item.
	 */
	uint32_t item_size;

	/**
	 * Total items used which includes items in queue and items popped from queue.
	 */
	uint32_t total;

	/**
	 * Internal queue flags.
	 */
	uint32_t flags;
} as_queue;

/******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * Initialize a stack allocated as_queue, with item storage on the stack.
 * as_queue_inita() will transfer stack memory to the heap if a resize is
 * required.
 */
#define as_queue_inita(__q, __item_size, __capacity)\
(__q)->data = alloca((__capacity) * (__item_size));\
(__q)->capacity = __capacity;\
(__q)->head = (__q)->tail = 0;\
(__q)->item_size = __item_size;\
(__q)->total = 0;\
(__q)->flags = 0;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Initialize a stack allocated as_queue, with item storage on the heap.
 */
AS_EXTERN bool
as_queue_init(as_queue* queue, uint32_t item_size, uint32_t capacity);

/**
 * Create a heap allocated as_queue, with item storage on the heap.
 */
AS_EXTERN as_queue*
as_queue_create(uint32_t item_size, uint32_t capacity);

/**
 * Release queue memory.
 */
AS_EXTERN void
as_queue_destroy(as_queue* queue);

/**
 * Get the number of items currently in the queue.
 */
static inline uint32_t
as_queue_size(as_queue* queue)
{
	return queue->tail - queue->head;
}

/**
 * Is queue empty?
 */
static inline bool
as_queue_empty(as_queue* queue)
{
	return queue->tail == queue->head;
}

/**
 * Push item to the tail of the queue.
 */
AS_EXTERN bool
as_queue_push(as_queue* queue, const void* ptr);

/**
 * Push item to the tail of the queue only if size < capacity.
 */
AS_EXTERN bool
as_queue_push_limit(as_queue* queue, const void* ptr);

/**
 * Push item to the head of the queue.
 */
AS_EXTERN bool
as_queue_push_head(as_queue* queue, const void* ptr);

/**
 * Push item to the head of the queue only if size < capacity.
 */
AS_EXTERN bool
as_queue_push_head_limit(as_queue* queue, const void* ptr);

/**
 * Get item at virtual index.  For internal use only.
 */
static inline void*
as_queue_get(as_queue* queue, uint32_t index)
{
	return &queue->data[(index % queue->capacity) * queue->item_size];
}

/**
 * Pop item from the head of the queue.
 */
static inline bool
as_queue_pop(as_queue* queue, void* ptr)
{
	if (as_queue_empty(queue)) {
		return false;
	}

	memcpy(ptr, as_queue_get(queue, queue->head), queue->item_size);
	queue->head++;

	// This probably keeps the cache fresher because the queue is fully empty.
	if (queue->head == queue->tail) {
		queue->head = queue->tail = 0;
	}
	return true;
}

/**
 * Pop item from the tail of the queue.
 */
static inline bool
as_queue_pop_tail(as_queue* queue, void* ptr)
{
	if (as_queue_empty(queue)) {
		return false;
	}

	queue->tail--;
	memcpy(ptr, as_queue_get(queue, queue->tail), queue->item_size);

	if (queue->head == queue->tail) {
		queue->head = queue->tail = 0;
	}
	return true;
}

/**
 * Increment total counter if within capacity.
 */
static inline bool
as_queue_incr_total(as_queue* queue)
{
	if (queue->total < queue->capacity) {
		queue->total++;
		return true;
	}
	return false;
}

/**
 * Decrement total counter.
 */
static inline void
as_queue_decr_total(as_queue* queue)
{
	queue->total--;
}

#ifdef __cplusplus
} // end extern "C"
#endif
