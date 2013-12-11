/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdarg.h>
#include <stdio.h>

#include <aerospike/as_logger.h>
#include <aerospike/as_util.h>

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated logger
 */
as_logger * as_logger_init(as_logger * logger, void * source, const as_logger_hooks * hooks) {
    if ( logger == NULL ) return logger;
    logger->source = source;
    logger->hooks = hooks;
    return logger;
}

/**
 * Heap allocate and initialize a logger
 */
as_logger * as_logger_new(void * source, const as_logger_hooks * hooks) {
    as_logger * logger = (as_logger *) cf_malloc(sizeof(as_logger));
    if (!logger) return logger;
    logger->source = source;
    logger->hooks = hooks;
    return logger;
}

/**
 * Release resources associated with the logger.
 * Calls logger->destroy. If success and if this is a heap allocated
 * logger, then it will be freed.
 */
int as_logger_destroy(as_logger * logger) {
    int rc = as_util_hook(destroy, 1, logger);
    if ( rc == 0 && logger->is_malloc ) {
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
bool as_logger_is_enabled(const as_logger * logger, const as_logger_level level) {
    return as_util_hook(enabled, false, logger, level);
}

/**
 * Get the current log level for the logger.
 */
as_logger_level as_logger_get_level(const as_logger * logger) {
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
int as_logger_log(const as_logger * logger, const as_logger_level level, const char * file, const int line, const char * format, ...) {
    va_list args;
    va_start(args, format);
    int rc = as_util_hook(log, 1, logger, level, file, line, format, args);
    va_end(args);
    return rc;
}
