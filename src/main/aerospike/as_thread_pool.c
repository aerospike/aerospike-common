/*
 * Copyright 2008-2022 Aerospike, Inc.
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
#include <aerospike/as_thread_pool.h>
#include <aerospike/as_atomic.h>
#include <aerospike/as_thread.h>
#include <citrusleaf/alloc.h>
#include <string.h>

//---------------------------------
// Types
//---------------------------------

typedef struct as_thread_pool_task_s {
	as_task_fn task_fn;
	void* task_data;
} as_thread_pool_task;

//---------------------------------
// Functions
//---------------------------------

void*
as_thread_worker(void* data)
{
	as_thread_pool* pool = data;

	as_thread_set_name("tpool");

	// Run variable tasks.
	as_thread_pool_task task;

	// Retrieve tasks from queue and execute.
	while (cf_queue_pop(pool->dispatch_queue, &task, CF_QUEUE_FOREVER) == CF_QUEUE_OK) {
		// A null task indicates thread should be shut down.
		if (! task.task_fn) {
			break;
		}

		// Run task
		task.task_fn(task.task_data);
	}

	// Run the finalization function, if present.
	if (pool->fini_fn) {
		pool->fini_fn();
	}
	return NULL;
}

int
as_thread_pool_init(as_thread_pool* pool, uint32_t thread_size)
{
	if (thread_size != 0) {
		pool->threads = cf_calloc(thread_size, sizeof(pthread_t));
		pool->dispatch_queue = cf_queue_create(sizeof(as_thread_pool_task), true);
	}
	else {
		pool->threads = NULL;
		pool->dispatch_queue = NULL;
	}
	pool->fini_fn = NULL;
	pool->thread_size = 0;

	// Start threads.
	for (uint32_t i = 0; i < thread_size; i++) {
		if (pthread_create(&pool->threads[i], NULL, as_thread_worker, pool) == 0) {
			pool->thread_size++;
		}
	}
	return (pool->thread_size == thread_size)? 0 : -3;
}

int
as_thread_pool_queue_task(as_thread_pool* pool, as_task_fn task_fn, void* task)
{
	if (pool->thread_size == 0) {
		// No threads are running to process task.
		return -1;
	}

	// Queue variable task.
	as_thread_pool_task vtask;
	vtask.task_fn = task_fn;
	vtask.task_data = task;

	if (cf_queue_push(pool->dispatch_queue, &vtask) != CF_QUEUE_OK) {
		return -2;
	}
	return 0;
}

int
as_thread_pool_destroy(as_thread_pool* pool)
{
	// Prevent double destroy.
	uint32_t thread_size = as_fas_uint32(&pool->thread_size, 0);

	if (thread_size == 0) {
		return 0;
	}

	// Tells worker threads to stop by setting NULL task_fn. We do this to allow the
	// workers to "wait forever" on processing the work dispatch queue, which has
	// minimum impact when the queue is empty. This also means all queued requests
	// get processed before shutting down.
	as_thread_pool_task task;
	task.task_fn = NULL;
	task.task_data = NULL;

	// Send shutdown signal for variable tasks.
	for (uint32_t i = 0; i < thread_size; i++) {
		cf_queue_push(pool->dispatch_queue, &task);
	}

	// Wait till threads finish.
	for (uint32_t i = 0; i < thread_size; i++) {
		pthread_join(pool->threads[i], NULL);
	}

	cf_free(pool->threads);
	cf_queue_destroy(pool->dispatch_queue);
	return 0;
}
