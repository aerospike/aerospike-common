#include "as_logger.h"
#include "as_util.h"
#include <stdarg.h>
#include <stdio.h>

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
    as_logger * logger = (as_logger *) malloc(sizeof(as_logger));
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
        free(logger);
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
 *       free(foo);
 *   }
 *
 */
bool as_logger_enabled(const as_logger * logger, const as_log_level level) {
    return as_util_hook(enabled, false, logger, level);
}

/**
 * Get the current log level for the logger.
 */
as_log_level as_logger_level(const as_logger * logger) {
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
int as_logger_log(const as_logger * logger, const as_log_level level, const char * file, const int line, const char * format, ...) {
    va_list args;
    va_start(args, format);
    int rc = as_util_hook(log, 1, logger, level, file, line, format, args);
    va_end(args);
    return rc;
}
