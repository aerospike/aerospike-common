#include "../test.h"

#include <aerospike/as_password.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( password_generation, "password gen" ) {
	
	// Generate random salt.
	char salt[AS_PASSWORD_HASH_SIZE];
	bool status = as_password_gen_salt(salt);
	assert(status);
	
	// Hash password using salt.
	char hash1[AS_PASSWORD_HASH_SIZE];
	status = as_password_gen_hash("mypass", salt, hash1);
	assert(status);
	
	// Salt is contained in hash. Therefore, hash can be used as identical salt.
	char hash2[AS_PASSWORD_HASH_SIZE];
	status = as_password_gen_hash("mypass", hash1, hash2);
	assert(status);
	
	// Verify password is same.
	status = as_password_verify(hash1, hash2);
	assert(status);
	
	// Generate hash for different password.
	status = as_password_gen_hash("mypass2", hash1, hash2);
	assert(status);
	
	// Verify password is different.
	status = as_password_verify(hash1, hash2);
	assert(! status);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( password, "password hash" ) {
    suite_add( password_generation );
}
