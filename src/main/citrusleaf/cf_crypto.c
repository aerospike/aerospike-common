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

/**
 * A general purpose hashtable implementation
 * Good at multithreading
 * Just, hopefully, the last reasonable hash table you'll ever need
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <openssl/sha.h>

#include <citrusleaf/cf_crypto.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * cf_convert_sha1_to_hex
 * Desc: Convert a sha1 returned hash to a hexadecimal string. 
 */
bool cf_convert_sha1_to_hex(unsigned char * hash, unsigned char * sha1_hex_buff) { 
        if (!sha1_hex_buff || !hash) return false; 
        for (unsigned int i = 0; i < SHA_DIGEST_LENGTH; i++) { 
                sprintf((char *)(sha1_hex_buff + (i * 2)), "%02x", hash[i]); 
        } 
        return true; 
}

