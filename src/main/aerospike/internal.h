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

//
// logging
//

#ifndef LOG_ENABLED
#define LOG_ENABLED 0
#endif

#if LOG_ENABLED == 1

#define LOG(fmt, args...) \
    __log_append(__FILE__, __LINE__, fmt, ## args);

#define LOG_COND(cond, fmt, args...) \
    if ( cond ) { __log_append(__FILE__, __LINE__, fmt, ## args); }

void __log_append(const char * file, int line, const char * fmt, ...);

#define DO_PRAGMA(x) _Pragma (#x)
#define TODO(x) DO_PRAGMA(message ("TODO - " #x))

#else

#define LOG(fmt, args...) 

#define LOG_COND(cond, fmt, args...) 

#endif
