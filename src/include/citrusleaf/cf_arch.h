/* 
 * Copyright 2008-2014 Aerospike, Inc.
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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#if defined(__powerpc64__)
  #define MARCH_PPC64 1
#endif

#if ! (defined(MARCH_i686) || defined(MARCH_x86_64) || defined(MARCH_PPC64))
  #if defined(__LP64__) || defined(_LP64)
    #define MARCH_x86_64 1
  #else
    #define MARCH_i686 1
  #endif
#endif

/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
