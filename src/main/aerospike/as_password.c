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
#include <aerospike/as_password.h>
#include <aerospike/as_random.h>
#include <aerospike/as_string.h>
#include "crypt_blowfish.h"
#include <stdio.h>

#define BCRYPT_RAND_LEN 16
#define BCRYPT_WORK_FACTOR 10
#define BCRYPT_SALT "$2a$10$7EqJtq98hPqEX7fNZaFWoO"

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void
as_set_echo(bool echo)
{
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);

	if (echo)
	{
		mode |= ENABLE_ECHO_INPUT;
	}
	else
	{
		mode &= ~ENABLE_ECHO_INPUT;
	}
	SetConsoleMode(hStdin, mode);
}
#else
#include <termios.h>
#include <unistd.h>

static void
as_set_echo(bool echo)
{
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);

	if (echo)
	{
		tty.c_lflag |= ECHO;
	}
	else
	{
		tty.c_lflag &= ~ECHO;
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}
#endif

bool
as_password_gen_salt(char* salt)
{
	// Create BCrypt random salt.
	char rnd[BCRYPT_RAND_LEN];
	
	as_random_get_bytes((uint8_t*)rnd, BCRYPT_RAND_LEN);
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

bool
as_password_get_constant_hash(const char* password, char* hash)
{
	if (! password) {
		password = "";
	}
	
	return as_password_gen_constant_hash(password, hash);
}

static void
as_password_prompt(char* password, int size)
{
	// Turn off echo.
	as_set_echo(false);

	// Prompt for password.
	printf("Enter Password: ");
	fflush(stdout);

	// Read password until newline.
	char* s = fgets(password, size, stdin);

	if (s) {
		int len = (int)strlen(password);

		if (password[len - 1] == '\n') {
			password[--len] = 0;
		}
	}
	else {
		password[0] = 0;
	}

	// Restore echo.
	as_set_echo(true);
	printf("\n");
}

bool
as_password_prompt_hash(const char* password, char* hash)
{
	char pass[AS_PASSWORD_HASH_SIZE];
	
	if (password && *password) {
		as_strncpy(pass, password, sizeof(pass));
	}
	else {
		as_password_prompt(pass, sizeof(pass));
	}
	
	return as_password_get_constant_hash(pass, hash);
}

void
as_password_acquire(char* password_trg, const char* password_src, int size)
{
	if (password_src && *password_src) {
		as_strncpy(password_trg, password_src, size);
	}
	else {
		as_password_prompt(password_trg, size);
	}
}
