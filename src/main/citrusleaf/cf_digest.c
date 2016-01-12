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

#include <stdint.h>
#include <stdio.h>

#include <citrusleaf/cf_digest.h>

void
cf_digest_string(cf_digest *digest, char* output)
{
    uint8_t *d = (uint8_t *) digest;
    char* p = output;

    *p++ = '0';
    *p++ = 'x';

    for (int i = 0; i < CF_DIGEST_KEY_SZ; i++) {
        sprintf(p, "%02x", d[i]);
        p += 2;
    }
}
