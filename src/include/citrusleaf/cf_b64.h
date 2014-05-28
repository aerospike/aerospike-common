/* 
 * Copyright 2008-2014 Aerospike, Inc.
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
#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

bool cf_base64_validate_input(const uint8_t *b, const int len);
int cf_base64_encode_maxlen(int len);
void cf_base64_encode(uint8_t * in_bytes, uint8_t *out_bytes, int *len_r);
void cf_base64_tostring(uint8_t * in_bytes, char *out_bytes, int *len_r);
int cf_base64_decode_inplace(uint8_t *bytes, int *len_r, bool validate);
int cf_base64_decode(uint8_t *in_bytes, uint8_t *out_bytes, int *len_r, bool validate);
int cf_base64_test();


/******************************************************************************/

#ifdef __cplusplus
} // end extern "C"
#endif
