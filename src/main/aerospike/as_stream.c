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
#include <aerospike/as_stream.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_util.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 *  Wrapper functions to ensure each CF allocation-related function call has a unique line.
 */
void *as_stream_malloc(size_t size)
{
	return cf_malloc(size);
}

void as_stream_free(void *ptr)
{
	cf_free(ptr);
}
