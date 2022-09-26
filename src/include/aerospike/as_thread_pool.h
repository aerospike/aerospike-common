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
#pragma once

#include <aerospike/as_std.h>
#include <citrusleaf/cf_queue.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
	
//---------------------------------
// Types
//---------------------------------

/**
 *	@private
 *	Task function callback.
 */
typedef void (*as_task_fn)(void* user_data);
	
/**
 *	@private
 *	Thread finalization function callback.
 */
typedef void (*as_fini_fn)(void);
	
/**
 *	@private
 *	Thread pool.
 */
typedef struct as_thread_pool_s {
	pthread_t* threads;
	cf_queue* dispatch_queue;
	as_fini_fn fini_fn;
	uint32_t thread_size;
} as_thread_pool;

//---------------------------------
// Functions
//---------------------------------

/**
 *	@private
 *	Initialize variable task thread pool and start thread_size threads.
 *	Multiple task types can be handled in variable task thread pools.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Failed to initialize mutex lock
 *	-2 : Failed to lock mutex
 *	-3 : Some threads failed to start
 */
int
as_thread_pool_init(as_thread_pool* pool, uint32_t thread_size);

/**
 *	@private
 *	Queue a variable task onto thread pool.
 *
 *	Returns:
 *	0  : Success
 *	-1 : No threads are running to process task.
 *	-2 : Failed to push task onto dispatch queue
 */
int
as_thread_pool_queue_task(as_thread_pool* pool, as_task_fn task_fn, void* task);

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
