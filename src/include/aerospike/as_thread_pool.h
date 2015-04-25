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
#pragma once

#include <citrusleaf/cf_queue.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
	
/******************************************************************************
 *	TYPES
 *****************************************************************************/
	
/**
 *	@private
 *	Thread pool.
 */
struct as_thread_pool {
	pthread_mutex_t lock;
	pthread_t* threads;
	cf_queue* dispatch_queue;
	uint32_t thread_capacity;
	uint32_t thread_size;
};
	
typedef struct as_thread_pool as_thread_pool;
	
/**
 *	@private
 *	Task function callback.
 */
typedef void (*as_task_fn)(void* user_data);

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Initialize thread pool and start thread_capacity threads.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Failed to initialize mutex lock
 *	-2 : Failed to lock mutex
 */
int
as_thread_pool_init(as_thread_pool* pool, uint32_t thread_capacity);

/**
 *	@private
 *	Resize number of running threads in thread pool.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Failed to lock mutex
 *	-2 : Pool has already been closed
 */
int
as_thread_pool_resize(as_thread_pool* pool, uint32_t thread_size);

/**
 *	@private
 *	Queue a task onto thread pool.
 *
 *	Returns:
 *	0  : Success
 *	-1 : No threads are running to process task.
 *	-2 : Failed to push task onto dispatch queue
 */
int
as_thread_pool_queue_task(as_thread_pool* pool, as_task_fn task_fn, void* udata);

/**
 *	@private
 *	Destroy thread pool.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Failed to lock mutex
 *	-2 : Pool has already been closed
 */
int
as_thread_pool_destroy(as_thread_pool* pool);

#ifdef __cplusplus
} // end extern "C"
#endif
