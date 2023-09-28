/*
 * Copyright 2023 Aerospike, Inc.
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

#if defined _MSC_VER

#define as_arch_prefetch_nt(_p)
#define as_arch_pause() YieldProcessor()

#else // gcc & clang

#define as_arch_compiler_barrier() asm volatile ("" : : : "memory")

#define as_arch_prefetch_nt(_p) __builtin_prefetch((_p), 0, 0)

#if defined __x86_64__

#define as_arch_pause() asm volatile("pause" : : : "memory")

#elif defined __aarch64__

#define as_arch_pause() asm volatile("isb" : : : "memory")

#endif

#endif // gcc & clang
