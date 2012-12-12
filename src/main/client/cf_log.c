/*
 * Copyright 2012 Aerospike. All rights reserved.
 */

#include "client/cf_log.h"
#include "client/cf_atomic.h"

#include <stdio.h>
#include <string.h>


static void cf_default_log(cf_log_level level, const char* fmt_no_newline, ...) {
	size_t fmt_size = strlen(fmt_no_newline) + 2;
	char fmt[fmt_size];

	strncpy(fmt, fmt_no_newline, fmt_size);
	fmt[fmt_size - 2] = '\n';

	va_list ap;

	va_start(ap, fmt_no_newline);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

cf_atomic32 g_log_level = (cf_atomic32)CF_INFO;
cf_atomic_p g_log_callback = (cf_atomic_p)cf_default_log;