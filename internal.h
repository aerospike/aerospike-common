#pragma once

#include <stdio.h>
#include <stdarg.h>

#define LOG(fmt, args...) \
    // __log_append(__FILE__, __LINE__, fmt, ## args);

void __log_append(const char * file, int line, const char * fmt, ...);
