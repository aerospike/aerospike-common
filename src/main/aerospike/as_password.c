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
#include <aerospike/as_string.h>
#include <citrusleaf/cf_random.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include "crypt_blowfish.h"

#define BCRYPT_RAND_LEN 16
#define BCRYPT_WORK_FACTOR 10
#define BCRYPT_SALT "$2a$10$7EqJtq98hPqEX7fNZaFWoO"

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

bool
as_password_gen_constant_hash(const char* password, char* hash)
{
	// Create BCrypt password hash.
	return (bool) _crypt_blowfish_rn(password, BCRYPT_SALT, hash, AS_PASSWORD_HASH_SIZE);
}

static void
as_password_prompt(char* password, int size)
{
	// Turn off echo.
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
	
    struct termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	
	// Prompt for password.
	printf("Enter Password: ");
	fflush(stdout);
	
	// Read password until newline.
	fgets(password, size, stdin);
	int len = (int)strlen(password);
	
	if (password[len-1] == '\n') {
		password[--len] = 0;
	}
	
	// Restore echo.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	printf("\n");
}

bool
as_password_prompt_hash(const char* input, char* output)
{
	char password[AS_PASSWORD_HASH_SIZE];
	
	if (input && *input) {
		as_strncpy(password, input, sizeof(password));
	}
	else {
		as_password_prompt(password, sizeof(password));
	}
	
	if (strlen(password) == 60 && memcmp(password, "$2a$", 4)) {
		// Password already hashed.
		strcpy(output, password);
		return true;
	}
	else {
		// Hash password.
		return as_password_gen_constant_hash(password, output);
	}
}
