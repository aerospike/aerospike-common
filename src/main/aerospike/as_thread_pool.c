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
#include "aerospike/as_thread_pool.h"
#include <citrusleaf/alloc.h>
#include <stdlib.h>
#include <unistd.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_thread_pool_task {
	as_task_fn task_fn;
	void* udata;
};

typedef struct as_thread_pool_task as_thread_pool_task;

/******************************************************************************
 * Functions
 *****************************************************************************/

static void
as_thread_pool_shutdown_threads(as_thread_pool* pool, uint32_t begin)
{
	// This tells the worker threads to stop. We do this (instead of using a
	// "running" flag) to allow the workers to "wait forever" on processing the
	// work dispatch queue, which has minimum impact when the queue is empty.
	// This also means all queued requests get processed when shutting down.
	for (uint32_t i = begin; i < pool->thread_size; i++) {
		as_thread_pool_task task;
		task.task_fn = NULL;
		task.udata = NULL;
		cf_queue_push(pool->dispatch_queue, &task);
	}

	// Wait till threads finish.
	for (uint32_t i = begin; i < pool->thread_size; i++) {
		pthread_join(pool->threads[i], NULL);
	}
}

static void*
as_thread_worker(void* data)
{
	as_thread_pool* pool = data;
	as_thread_pool_task task;
	
	while (cf_queue_pop(pool->dispatch_queue, &task, CF_QUEUE_FOREVER) == CF_QUEUE_OK) {
		// A null task indicates thread should be shut down.
		if (! task.task_fn) {
			break;
		}

		// Run task
		task.task_fn(task.udata);
	}
	return 0;
}

int
as_thread_pool_init(as_thread_pool* pool, uint32_t thread_capacity)
{
	if (pthread_mutex_init(&pool->lock, NULL)) {
		return -1;
	}

    if (pthread_mutex_lock(&pool->lock)) {
        return -2;
    }

	pool->dispatch_queue = cf_queue_create(sizeof(as_thread_pool_task), true);

	if (thread_capacity > 0) {
		// Allocate thread_capacity threads.
		pool->threads = cf_malloc(sizeof(pthread_t) * thread_capacity);
		pool->thread_capacity = thread_capacity;
		pool->thread_size = thread_capacity;
	}
	else {
		// Allocate 1 thread, but do not start it.
		pool->threads = cf_malloc(sizeof(pthread_t));
		pool->thread_capacity = 1;
		pool->thread_size = 0;
	}
	
	for (uint32_t i = 0; i < pool->thread_size; i++) {
		pthread_create(&pool->threads[i], 0, as_thread_worker, pool);
	}
	pthread_mutex_unlock(&pool->lock);
	return 0;
}

int
as_thread_pool_resize(as_thread_pool* pool, uint32_t thread_size)
{
	if (thread_size == pool->thread_size) {
		return 0;
	}
	
    if (pthread_mutex_lock(&pool->lock)) {
        return -1;
    }

	if (pool->threads == 0) {
		// Pool has already been closed.
		pthread_mutex_unlock(&pool->lock);
		return -2;
	}

	if (thread_size < pool->thread_size) {
		// Reduce thread pool size.
		as_thread_pool_shutdown_threads(pool, thread_size);
		pool->thread_size = thread_size;
		pthread_mutex_unlock(&pool->lock);
		return 0;
	}
	
	if (thread_size <= pool->thread_capacity) {
		// Increase thread pool size within current capacity.
		for (uint32_t i = pool->thread_size; i < thread_size; i++) {
			pthread_create(&pool->threads[i], 0, as_thread_worker, pool);
		}
		pool->thread_size = thread_size;
		pthread_mutex_unlock(&pool->lock);
		return 0;
	}
	
	// Increase thread pool size past current capacity.
	pthread_t* threads_new = cf_malloc(sizeof(pthread_t) * thread_size);
	memcpy(threads_new, pool->threads, sizeof(pthread_t) * pool->thread_size);
	
	for (uint32_t i = pool->thread_size; i < thread_size; i++) {
		pthread_create(&threads_new[i], 0, as_thread_worker, pool);
	}
	
	pthread_t* tmp = pool->threads;
	pool->threads = threads_new;
	pool->thread_capacity = thread_size;
	pool->thread_size = thread_size;
	cf_free(tmp);
	pthread_mutex_unlock(&pool->lock);
	return 0;
}

int
as_thread_pool_queue_task(as_thread_pool* pool, as_task_fn task_fn, void* udata)
{
	if (pool->thread_size == 0) {
		return -1;
	}
	
	as_thread_pool_task task;
	task.task_fn = task_fn;
	task.udata = udata;
	
	if (cf_queue_push(pool->dispatch_queue, &task) != CF_QUEUE_OK) {
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

	if (pool->threads == 0) {
		// Pool has already been closed.
		pthread_mutex_unlock(&pool->lock);
		return -2;
	}

	as_thread_pool_shutdown_threads(pool, 0);
	cf_queue_destroy(pool->dispatch_queue);
	
	cf_free(pool->threads);
	pool->threads = 0;
	
	pthread_mutex_unlock(&pool->lock);
	pthread_mutex_destroy(&pool->lock);
	return 0;
}
