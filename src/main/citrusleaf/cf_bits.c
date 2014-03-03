/*
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

#include <citrusleaf/cf_bits.h>
#include <citrusleaf/cf_clock.h>
#include <citrusleaf/cf_atomic.h>

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
        return cf_bits_find_last_set((uint32_t)t) + 32;
    }
    else {
        return cf_bits_find_last_set((uint32_t)v);
    }
}
