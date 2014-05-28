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

#include <stdarg.h>
#include <stdio.h>

#include <citrusleaf/alloc.h>

#include <aerospike/as_logger.h>
#include <aerospike/as_util.h>

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Constructs a logger
 */
static as_logger * as_logger_cons(as_logger * logger, bool free, void * source, const as_logger_hooks * hooks) 
{
	if ( logger == NULL ) return logger;

	logger->free = free;
	logger->source = source;
	logger->hooks = hooks;

	return logger;
}
/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated logger
 */
as_logger * as_logger_init(as_logger * logger, void * source, const as_logger_hooks * hooks) 
{
	return as_logger_cons(logger, false, source, hooks);
}

/**
 * Heap allocate and initialize a logger
 */
as_logger * as_logger_new(void * source, const as_logger_hooks * hooks) 
{
	as_logger * logger = (as_logger *) cf_malloc(sizeof(as_logger));
	if ( !logger ) return logger;
	return as_logger_cons(logger, true, source, hooks);
}

/**
 * Release resources associated with the logger.
 * Calls logger->destroy. If success and if this is a heap allocated
 * logger, then it will be freed.
 */
int as_logger_destroy(as_logger * logger) 
{
	int rc = as_util_hook(destroy, 1, logger);
	if ( rc == 0 && logger->free ) {
		cf_free(logger);
	}
	return rc;
}


/**
 * Test if the log level is enabled for the logger.
 *
 * For most purposes, you should use the macros:
 *   - as_logger_trace_enabled(logger)
 *   - as_logger_debug_enabled(logger)
 *   - as_logger_info_enabled(logger)
 *   - as_logger_warn_enabled(logger)
 *   - as_logger_error_enabled(logger)
 * 
 * Usage:
 *   if ( as_logger_enabled(logger, AS_LOG_DEBUG) ) {
 *       char * foo = tostring(x);
 *       as_logger_debug(logger, "foo = %s", foo);
 *       cf_free(foo);
 *   }
 *
 */
bool as_logger_is_enabled(const as_logger * logger, const as_logger_level level)
{
	return as_util_hook(enabled, false, logger, level);
}

/**
 * Get the current log level for the logger.
 */
as_logger_level as_logger_get_level(const as_logger * logger)
{
	return as_util_hook(level, 1, logger);
}

/**
 * Log a message using the logger.
 *
 * For most purposes, you should use the macros:
 *   - as_logger_trace(logger, message, ...)
 *   - as_logger_debug(logger, message, ...)
 *   - as_logger_info_(logger, message, ...)
 *   - as_logger_warn_(logger, message, ...)
 *   - as_logger_error(logger, message, ...)
 * 
 * Usage:
 *   as_logger_log(logger, AS_LOG_DEBUG, __FILE__, __LINE__, "Hello %s", "Bob");
 *
 */
int as_logger_log(const as_logger * logger, const as_logger_level level, const char * file, const int line, const char * format, ...)
{
	va_list args;
	va_start(args, format);
	int rc = as_util_hook(log, 1, logger, level, file, line, format, args);
	va_end(args);
	return rc;
}
