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
#include <aerospike/as_log.h>

/******************************************************************************
 *	GLOBAL VARIABLES
 *****************************************************************************/

as_log g_as_log = {AS_LOG_LEVEL_INFO, 0};

const char* as_log_level_strings[] = {
	[AS_LOG_LEVEL_ERROR]	= "ERROR",
	[AS_LOG_LEVEL_WARN]		= "WARN",
	[AS_LOG_LEVEL_INFO]		= "INFO",
	[AS_LOG_LEVEL_DEBUG]	= "DEBUG",
	[AS_LOG_LEVEL_TRACE]	= "TRACE"
};
