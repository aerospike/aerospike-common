#include <citrusleaf/cf_clock.h>
#include <citrusleaf/cf_thread.h>

#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define ITERATIONS 100000000

static pthread_mutex_t m1, m2, m3, m4, m5, m6, m7, m8, m9;

// contends with bench3() for m1, m2, m3

void *bench1(void *arg)
{
	(void)arg;

	sleep(1);
	printf("benchmarking pthread_mutex_*()\n");

	clock_t start = cf_getms();

	for (int i = 0; i < ITERATIONS; ++i) {
		assert(pthread_mutex_lock(&m1) == 0);
		assert(pthread_mutex_lock(&m2) == 0);
		assert(pthread_mutex_lock(&m3) == 0);
		assert(pthread_mutex_unlock(&m3) == 0);
		assert(pthread_mutex_unlock(&m2) == 0);
		assert(pthread_mutex_unlock(&m1) == 0);
	}

	clock_t end = cf_getms();
	printf("pthread_mutex_*() took %d ms\n", (int)(end - start));
	return NULL;
}

// no contention, uses m4, m5, m6

void *bench2(void *arg)
{
	(void)arg;

	sleep(1);
	printf("benchmarking uncontended pthread_mutex_*()\n");

	clock_t start = cf_getms();

	for (int i = 0; i < ITERATIONS; ++i) {
		assert(pthread_mutex_lock(&m4) == 0);
		assert(pthread_mutex_lock(&m5) == 0);
		assert(pthread_mutex_lock(&m6) == 0);
		assert(pthread_mutex_unlock(&m6) == 0);
		assert(pthread_mutex_unlock(&m5) == 0);
		assert(pthread_mutex_unlock(&m4) == 0);
	}

	clock_t end = cf_getms();
	printf("uncontended pthread_mutex_*() took %d ms\n", (int)(end - start));
	return NULL;
}

// contends with bench1() for m1, m2, m3

void *bench3(void *arg)
{
	(void)arg;

	sleep(1);
	printf("benchmarking cf_mutex_*()\n");

	clock_t start = cf_getms();

	for (int i = 0; i < ITERATIONS; ++i) {
		assert(pthread_mutex_lock(&m1) == 0);
		assert(pthread_mutex_lock(&m2) == 0);
		assert(pthread_mutex_lock(&m3) == 0);
		assert(pthread_mutex_unlock(&m3) == 0);
		assert(pthread_mutex_unlock(&m2) == 0);
		assert(pthread_mutex_unlock(&m1) == 0);
	}

	clock_t end = cf_getms();
	printf("cf_mutex_*() took %d ms\n", (int)(end - start));
	return NULL;
}

// no contention, uses m7, m8, m9

void *bench4(void *arg)
{
	(void)arg;

	sleep(1);
	printf("benchmarking uncontended cf_mutex_*()\n");

	clock_t start = cf_getms();

	for (int i = 0; i < ITERATIONS; ++i) {
		assert(pthread_mutex_lock(&m7) == 0);
		assert(pthread_mutex_lock(&m8) == 0);
		assert(pthread_mutex_lock(&m9) == 0);
		assert(pthread_mutex_unlock(&m9) == 0);
		assert(pthread_mutex_unlock(&m8) == 0);
		assert(pthread_mutex_unlock(&m7) == 0);
	}

	clock_t end = cf_getms();
	printf("uncontended cf_mutex_*() took %d ms\n", (int)(end - start));
	return NULL;
}

// deadlocks with dead2, acquires m3, m4, m1, m2

void *dead1(void *arg)
{
	(void)arg;

	assert(cf_mutex_lock(&m3) == 0);
	assert(cf_mutex_lock(&m4) == 0);
	assert(cf_mutex_lock(&m1) == 0);
	// make sure that we deadlock
	sleep(1);
	assert(cf_mutex_lock(&m2) == 0);

	return NULL;
}

// deadlocks with dead1, acquires m5, m6, m2, m1

void *dead2(void *arg)
{
	(void)arg;

	assert(cf_mutex_lock(&m5) == 0);
	assert(cf_mutex_lock(&m6) == 0);
	assert(cf_mutex_lock(&m2) == 0);
	// make sure that we deadlock
	sleep(1);
	assert(cf_mutex_lock(&m1) == 0);

	return NULL;
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	assert(pthread_mutex_init(&m1, NULL) == 0);
	assert(pthread_mutex_init(&m2, NULL) == 0);
	assert(pthread_mutex_init(&m3, NULL) == 0);

	assert(pthread_mutex_init(&m4, NULL) == 0);
	assert(pthread_mutex_init(&m5, NULL) == 0);
	assert(pthread_mutex_init(&m6, NULL) == 0);

	assert(pthread_mutex_init(&m7, NULL) == 0);
	assert(pthread_mutex_init(&m8, NULL) == 0);
	assert(pthread_mutex_init(&m9, NULL) == 0);

	printf("------ benchmarking\n");

	pthread_t t1, t2, t3, t4;

	assert(cf_thread_create(&t1, NULL, bench1, NULL) == 0);
	assert(cf_thread_create(&t2, NULL, bench2, NULL) == 0);
	assert(cf_thread_create(&t3, NULL, bench3, NULL) == 0);
	assert(cf_thread_create(&t4, NULL, bench4, NULL) == 0);

	assert(pthread_join(t1, NULL) == 0);
	assert(pthread_join(t2, NULL) == 0);
	assert(pthread_join(t3, NULL) == 0);
	assert(pthread_join(t4, NULL) == 0);

	printf("------ creating deadlock\n");

	assert(cf_thread_create(&t1, NULL, dead1, NULL) == 0);
	assert(cf_thread_create(&t2, NULL, dead2, NULL) == 0);

	sleep(2);
	cf_mutex_dump();

	printf("------ deadlocked, press ctrl-C\n");

	assert(pthread_join(t1, NULL) == 0);
	assert(pthread_join(t2, NULL) == 0);

	return 0;
}
