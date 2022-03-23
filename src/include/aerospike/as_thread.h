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

#include <pthread.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AS_THREAD_NAME_MAX 16

/**
 * Assign thread name. Must be called in the thread to be named.
 */
static inline void
as_thread_set_name(const char* name)
{
#if defined(__APPLE__)
	pthread_setname_np(name);
#else
	pthread_setname_np(pthread_self(), name);
#endif
}

/**
 * Assign thread name with index suffix. Must be called in the thread to be named.
 */
static inline void
as_thread_set_name_index(const char* name, uint32_t index)
{
	char buf[AS_THREAD_NAME_MAX];
	snprintf(buf, sizeof(buf), "%s%u", name, index);
	as_thread_set_name(buf);
}

#ifdef __cplusplus
} // end extern "C"
#endif
