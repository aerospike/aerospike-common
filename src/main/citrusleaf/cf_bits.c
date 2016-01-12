/* 
 * Copyright 2008-2016 Aerospike, Inc.
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
