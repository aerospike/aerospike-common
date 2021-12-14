/* 
 * Copyright 2008-2021 Aerospike, Inc.
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

//==========================================================
// Includes.
//

#include <aerospike/as_std.h>

#ifdef __cplusplus
extern "C" {
#endif



/*
 * Copy of SHA1 API and implementation.
 *
 * For FIPS 140 compliance, this is NOT to be used for crypto purposes!
 */

//==========================================================
// Typedefs & constants.
//

#define CF_SHA_DIGEST_LENGTH 20


//==========================================================
// Public API.
//

void cf_SHA1(const uint8_t* d, size_t n, uint8_t* md);



/*
 * Copy of RIPEMD160 API and implementation.
 *
 * For FIPS 140 compliance, this is NOT to be used for crypto purposes!
 */

//==========================================================
// Typedefs & constants.
//

#define CF_RIPEMD160_DIGEST_LENGTH 20

// Private! Do not access members.
typedef struct cf_RIPEMD160_CTX_s {
	uint32_t total[2];
	uint32_t state[5];
	unsigned char buffer[64];
} cf_RIPEMD160_CTX;


//==========================================================
// Public API.
//

int cf_RIPEMD160(const unsigned char* input, size_t ilen, unsigned char* output);
int cf_RIPEMD160_Init(cf_RIPEMD160_CTX* ctx);
int cf_RIPEMD160_Update(cf_RIPEMD160_CTX* ctx, const void* v_input, size_t ilen);
int cf_RIPEMD160_Final(unsigned char* output, cf_RIPEMD160_CTX* ctx);


#ifdef __cplusplus
} // end extern "C"
#endif
