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
#include "aerospike/as_thread_pool.h"
#include <citrusleaf/alloc.h>
#include <stdlib.h>
#include <unistd.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

typedef struct as_thread_pool_task_s {
	as_task_fn task_fn;
	void* task_data;
} as_thread_pool_task;

/******************************************************************************
 * Functions
 *****************************************************************************/

void*
as_thread_worker(void* data)
{
	as_thread_pool* pool = data;
	
	if (pool->task_size == 0) {
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
	}
	else {
		// Run fixed tasks.
		char* task = alloca(pool->task_size);
		bool* shutdown = (bool*)(task + pool->task_complete_offset);
		
		// Retrieve tasks from queue and execute.
		while (cf_queue_pop(pool->dispatch_queue, task, CF_QUEUE_FOREVER) == CF_QUEUE_OK) {
			// Check if thread should be shut down.
			if (*shutdown) {
				break;
			}
			
			// Run task
			pool->task_fn(task);
		}
	}
	
	// Send thread completion event back to caller.
	uint32_t complete = 1;
	cf_queue_push(pool->complete_queue, &complete);
	return 0;
}

static uint32_t
as_thread_pool_create_threads(as_thread_pool* pool, uint32_t count)
{
	// Start detached threads.  There is no need to store thread because a completion queue
	// is used to join back threads on termination.
	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	
	uint32_t threads_created = 0;
	pthread_t thread;
	
	for (uint32_t i = 0; i < count; i++) {
		if (pthread_create(&thread, &attrs, as_thread_worker, pool) == 0) {
			threads_created++;
		}
	}
	return threads_created;
}

static void
as_thread_pool_shutdown_threads(as_thread_pool* pool, uint32_t count)
{
	// This tells the worker threads to stop. We do this (instead of using a
	// "running" flag) to allow the workers to "wait forever" on processing the
	// work dispatch queue, which has minimum impact when the queue is empty.
	// This also means all queued requests get processed when shutting down.
	if (pool->task_size == 0) {
		// Send shutdown signal for variable tasks.
		as_thread_pool_task task;
		task.task_fn = NULL;
		task.task_data = NULL;

		for (uint32_t i = 0; i < count; i++) {
			cf_queue_push(pool->dispatch_queue, &task);
		}
	}
	else {
		// Send shutdown signal for fixed tasks.
		char* task = alloca(pool->task_size);
		memset(task, 0, pool->task_size);
		*(bool*)(task + pool->task_complete_offset) = true;
		
		for (uint32_t i = 0; i < count; i++) {
			cf_queue_push(pool->dispatch_queue, task);
		}
	}
	
	// Wait till threads finish.
	uint32_t complete;
	for (uint32_t i = 0; i < count; i++) {
		cf_queue_pop(pool->complete_queue, &complete, CF_QUEUE_FOREVER);
	}
}

int
as_thread_pool_init(as_thread_pool* pool, uint32_t thread_size)
{
	if (pthread_mutex_init(&pool->lock, NULL)) {
		return -1;
	}

    if (pthread_mutex_lock(&pool->lock)) {
        return -2;
    }

	// Initialize queues.
	pool->dispatch_queue = cf_queue_create(sizeof(as_thread_pool_task), true);
	pool->complete_queue = cf_queue_create(sizeof(uint32_t), true);
	pool->task_fn = 0;
	pool->task_size = 0;
	pool->task_complete_offset = 0;
	pool->thread_size = thread_size;
	pool->initialized = 1;
	
	// Start detached threads.
	pool->thread_size = as_thread_pool_create_threads(pool, thread_size);
	int rc = (pool->thread_size == thread_size)? 0 : -3;
	
	pthread_mutex_unlock(&pool->lock);
	return rc;
}

int
as_thread_pool_init_fixed(as_thread_pool* pool, uint32_t thread_size, as_task_fn task_fn,
						  uint32_t task_size, uint32_t task_complete_offset)
{
	if (pthread_mutex_init(&pool->lock, NULL)) {
		return -1;
	}
	
    if (pthread_mutex_lock(&pool->lock)) {
        return -2;
    }
	
	// Initialize queues.
	pool->dispatch_queue = cf_queue_create(task_size, true);
	pool->complete_queue = cf_queue_create(sizeof(uint32_t), true);
	pool->task_fn = task_fn;
	pool->task_size = task_size;
	pool->task_complete_offset = task_complete_offset;
	pool->thread_size = thread_size;
	pool->initialized = 1;
	
	// Start detached threads.
	pool->thread_size = as_thread_pool_create_threads(pool, thread_size);
	int rc = (pool->thread_size == thread_size)? 0 : -3;
	
	pthread_mutex_unlock(&pool->lock);
	return rc;
}

int
as_thread_pool_resize(as_thread_pool* pool, uint32_t thread_size)
{
    if (pthread_mutex_lock(&pool->lock)) {
        return -1;
    }

	if (! pool->initialized) {
		// Pool has already been closed.
		pthread_mutex_unlock(&pool->lock);
		return -2;
	}
	
	int rc = 0;
	
	if (thread_size != pool->thread_size) {
		if (thread_size < pool->thread_size) {
			// Shutdown excess threads.
			uint32_t threads_to_shutdown = pool->thread_size - thread_size;
			
			// Set pool thread_size before shutting down threads because we want to disallow
			// new tasks onto thread pool when thread_size is set to zero. Therefore, set
			// thread_size as early as possible.
			// Note: There still is a slight possibility that new tasks can still be queued
			// after disabliing thread pool because the thread_size check is not done under lock.
			// These tasks will either timeout or be suspended until thread pool is resized to > 0
			// threads.
			pool->thread_size = thread_size;
			
			as_thread_pool_shutdown_threads(pool, threads_to_shutdown);
		}
		else {
			// Start new threads.
			pool->thread_size += as_thread_pool_create_threads(pool, thread_size - pool->thread_size);
			rc = (pool->thread_size == thread_size)? 0 : -3;
		}
	}
	pthread_mutex_unlock(&pool->lock);
	return rc;
}

int
as_thread_pool_queue_task(as_thread_pool* pool, as_task_fn task_fn, void* task)
{
	if (pool->thread_size == 0) {
		// No threads are running to process task.
		return -1;
	}
	
	if (pool->task_size == 0) {
		// Queue variable task.
		as_thread_pool_task vtask;
		vtask.task_fn = task_fn;
		vtask.task_data = task;
		
		if (cf_queue_push(pool->dispatch_queue, &vtask) != CF_QUEUE_OK) {
			return -2;
		}
	}
	else {
		// Query fixed task.
		if (cf_queue_push(pool->dispatch_queue, task) != CF_QUEUE_OK) {
			return -2;
		}
	}
	return 0;
}

int
as_thread_pool_queue_task_fixed(as_thread_pool* pool, void* task)
{
	if (pool->thread_size == 0) {
		// No threads are running to process task.
		return -1;
	}
	
	if (cf_queue_push(pool->dispatch_queue, task) != CF_QUEUE_OK) {
		return -2;
	}
	return 0;
}

int
as_thread_pool_destroy(as_thread_pool* pool)
{
	if (pthread_mutex_lock(&pool->lock)) {
		return -1;
	}

	if (! pool->initialized) {
		// Pool has already been closed.
		pthread_mutex_unlock(&pool->lock);
		return -2;
	}

	as_thread_pool_shutdown_threads(pool, pool->thread_size);
	cf_queue_destroy(pool->dispatch_queue);
	cf_queue_destroy(pool->complete_queue);
	pool->initialized = 0;
	pthread_mutex_unlock(&pool->lock);
	pthread_mutex_destroy(&pool->lock);
	return 0;
}
