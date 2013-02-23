#pragma once

//
// logging
//

#define LOG(fmt, args...) \
    // __log_append(__FILE__, __LINE__, fmt, ## args);

void __log_append(const char * file, int line, const char * fmt, ...);

#define DO_PRAGMA(x) _Pragma (#x)
#define TODO(x) DO_PRAGMA(message ("TODO - " #x))