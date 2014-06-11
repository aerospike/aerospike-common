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
#include <aerospike/as_password.h>
#include <citrusleaf/cf_random.h>
#include "crypt_blowfish.h"

#define BCRYPT_RAND_LEN 16
#define BCRYPT_WORK_FACTOR 10

bool
as_password_gen_salt(char* salt)
{
	// Create BCrypt random salt.
	char rnd[BCRYPT_RAND_LEN];
	
	cf_get_rand_buf((uint8_t*)rnd, BCRYPT_RAND_LEN);
	return (bool) _crypt_gensalt_blowfish_rn("$2a$", BCRYPT_WORK_FACTOR, rnd, BCRYPT_RAND_LEN, salt, AS_PASSWORD_HASH_SIZE);
}

bool
as_password_gen_hash(const char* password, const char* salt, char* hash)
{
	// Create BCrypt password hash.
	return (bool) _crypt_blowfish_rn(password, salt, hash, AS_PASSWORD_HASH_SIZE);
}
