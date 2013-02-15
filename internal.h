#pragma once

//
// logging
//

#define LOG(fmt, args...) \
    __log_append(__FILE__, __LINE__, fmt, ## args);

void __log_append(const char * file, int line, const char * fmt, ...);
