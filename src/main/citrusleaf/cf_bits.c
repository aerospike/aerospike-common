#include "citrusleaf/cf_bits.h"
#include "citrusleaf/cf_clock.h"
#include "citrusleaf/cf_atomic.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>


int cf_bits_find_last_set(uint32_t v) {
    uint32_t t, tt;
    if ((tt = v >> 16)) {
        return (t = tt >> 8) ? (24 + cf_LogTable256[t]) : (16 + cf_LogTable256[tt]);
    }
    else {
        return (t = v >> 8) ? (8 + cf_LogTable256[t]) : cf_LogTable256[v];
    }
}

int cf_bits_find_last_set_64(uint64_t v) {
    uint64_t t;
    if ((t = v >> 32)) {
        return cf_bits_find_last_set(t) + 32;
    }
    else {
        return cf_bits_find_last_set(v);
    }
}
