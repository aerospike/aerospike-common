#include "internal.h"

void __log_append(const char * file, int line, const char * fmt, ...) {
    char msg[128] = {0};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, 128, fmt, ap);
    va_end(ap);
    printf("%s:%d – %s\n",file,line,msg);
}
