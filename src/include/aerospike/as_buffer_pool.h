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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *	TYPES
 *****************************************************************************/

/**
 *	@private
 *	Buffer builder.
 */
typedef struct as_buffer_builder_s {
	uint8_t* data;
	uint32_t capacity;
	uint32_t size;
} as_buffer_builder;

/**
 *	@private
 *	Buffer pool.
 */
typedef struct as_buffer_pool_s {
	cf_queue* queue;
	uint32_t buffer_size;
	uint32_t request_max;
} as_buffer_pool;

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

/**
 *	@private
 *	Initialize empty buffer pool.  Each buffer in the pool will be a fixed size.
 *
 *	@param pool			Buffer pool.
 *	@param buffer_size 	Fixed buffer size.
 *	@param request_max	Maximum request size.  Set to zero if there is no maximum.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Failed to create queue.
 */
int
as_buffer_pool_init(as_buffer_pool* pool, uint32_t buffer_size, uint32_t request_max);

/**
 *	@private
 *	If requested buffer size is less than/equal the pool's buffer size, pop buffer from pool.
 *	Otherwise allocate memory on heap.  If the pool is empty, also create buffer on heap.
 *
 *	@param pool			Buffer pool.
 *	@param bb			Buffer to be populated.
 *	@param size			Requested size of buffer.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Requested size is out of bounds.
 *	-2 : Memory allocation error.
 *	-3 : Queue failure.
 */
int
as_buffer_pool_pop(as_buffer_pool* pool, as_buffer_builder* bb, uint32_t size);

/**
 *	@private
 *	If buffer capacity less than/equal the pool's buffer size, push buffer back into pool.
 *	Otherwise, free memory and do not put back into pool.
 *
 *	@param pool			Buffer pool.
 *	@param bb			Buffer.
 *
 *	Returns:
 *	0  : Success
 *	-1 : Queue failure.
 */
int
as_buffer_pool_push(as_buffer_pool* pool, as_buffer_builder* bb);
	
/**
 *	@private
 *	Reduce free buffer queue count to a more acceptable level.  Pop buffers off queue until queue 
 *	count has been reached.  This is useful when a large number of buffers are created due to a
 *	burst of concurrent buffer usage and pruning is desired.
 *
 *	@param pool			Buffer pool.
 *	@param queue_count	Desired number of buffers sitting in queue that are not currently used.
 *
 *	Returns number of buffers deleted.
 */
int
as_buffer_pool_trim(as_buffer_pool* pool, int queue_count);

/**
 *	@private
 *	Empty buffer pool and destroy.
 *
 *	@param pool			Buffer pool.
 */
void
as_buffer_pool_destroy(as_buffer_pool* pool);

#ifdef __cplusplus
} // end extern "C"
#endif
