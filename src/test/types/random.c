#include "../test.h"

#include <aerospike/as_random.h>
#include <pthread.h>
#include <string.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

static uint64_t n3, n4, n5, n6;

static void*
worker1(void* data)
{
	n3 = as_random_get_uint64();
	n4 = as_random_get_uint64();
	return NULL;
}

static void*
worker2(void* data)
{
	n5 = as_random_get_uint64();
	n6 = as_random_get_uint64();
	return NULL;
}

TEST(random_number, "random numbers" )
{
	pthread_t thread1;
	pthread_create(&thread1, NULL, worker1, NULL);
	
	pthread_t thread2;
	pthread_create(&thread2, NULL, worker2, NULL);
	
	uint64_t n1 = as_random_get_uint64();
	uint64_t n2 = as_random_get_uint64();
	
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	
	assert(n1 != n2 && n1 != n3 && n1 != n4 && n1 != n5 && n1 != n6);
	assert(n2 != n3 && n2 != n4 && n2 != n5 && n2 != n6);
	assert(n3 != n4 && n3 != n5 && n3 != n6);
	assert(n4 != n5 && n4 != n6);
	assert(n5 != n6);
}

TEST(random_bytes, "random numbers" )
{
	uint8_t b1[3];
	memset(b1, 0, sizeof(b1));
	
	as_random_get_bytes(b1, 0);
	assert(b1[0] == 0);
	
	as_random_get_bytes(b1, 2);
	assert(b1[2] == 0);
	
	uint8_t b2[64];
	memset(b2, 0, sizeof(b2));
	
	as_random_get_bytes(b2, 8);
	assert(b2[8] == 0);
	
	as_random_get_bytes(b2, 63);
	assert(b2[63] == 0);
}


/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE(random_numbers, "random number generator")
{
    suite_add(random_number);
    suite_add(random_bytes);
}
