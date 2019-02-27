#include "../test.h"

#include <aerospike/as_queue_mt.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST(types_queue_mt_stack, "as_queue_mt stack mode")
{
	// Initialize queue on stack.
    as_queue_mt v;
	as_queue_mt_inita(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_queue_mt_push(&v, &i);
	}
	
	assert(as_queue_mt_size(&v) == 9);
	assert(v.queue.capacity == 10);
	assert(v.queue.flags == 0);
	
	// Add more items so queue is resized/converted to heap.
	int i = 9;
	as_queue_mt_push(&v, &i);
	i = 10;
	as_queue_mt_push(&v, &i);
	
	assert(as_queue_mt_size(&v) == 11);
	assert(v.queue.capacity == 20);
	assert(v.queue.flags == 1);
	
	int result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_mt_pop(&v, &result, AS_QUEUE_NOWAIT)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}
	
	assert(as_queue_mt_size(&v) == 0);
	assert(v.queue.capacity == 20);
	assert(v.queue.flags == 1);
	
	// Destroy queue.
    as_queue_mt_destroy(&v);
}

TEST(types_queue_mt_heap_init, "as_queue_mt heap with init")
{
	// Initialize array on heap.
    as_queue_mt v;
	as_queue_mt_init(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_queue_mt_push(&v, &i);
	}
	
	assert(as_queue_mt_size(&v) == 9);
	assert(v.queue.capacity == 10);
	assert(v.queue.flags == 1);
		
	// Add more items so queue is resized.
	int i = 9;
	as_queue_mt_push(&v, &i);
	i = 10;
	as_queue_mt_push(&v, &i);
	
	assert(as_queue_mt_size(&v) == 11);
	assert(v.queue.capacity == 20);
	assert(v.queue.flags == 1);
	
	int result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_mt_pop(&v, &result, AS_QUEUE_NOWAIT)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}
	
	assert(as_queue_mt_size(&v) == 0);
	assert(v.queue.capacity == 20);
	assert(v.queue.flags == 1);

	// Destroy queue.
    as_queue_mt_destroy(&v);
}

TEST(types_queue_mt_heap_create, "as_queue_mt heap with create")
{
	// Initialize array on heap.
    as_queue_mt* v = as_queue_mt_create(sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_queue_mt_push(v, &i);
	}
	
	assert(as_queue_mt_size(v) == 9);
	assert(v->queue.capacity == 10);
	assert(v->queue.flags == 3);
		
	// Add more items so queue is resized.
	int i = 9;
	as_queue_mt_push(v, &i);
	i = 10;
	as_queue_mt_push(v, &i);
	
	assert(as_queue_mt_size(v) == 11);
	assert(v->queue.capacity == 20);
	assert(v->queue.flags == 3);
	
	int result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_mt_pop(v, &result, AS_QUEUE_NOWAIT)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}
	
	assert(as_queue_mt_size(v) == 0);
	assert(v->queue.capacity == 20);
	assert(v->queue.flags == 3);

	// Destroy queue.
    as_queue_mt_destroy(v);
}

TEST(types_queue_mt_pointers, "as_queue_mt pointer elements")
{
    as_queue_mt v;
	as_queue_mt_init(&v, sizeof(int*), 10);
	
	for (int i = 0; i < 11; i++) {
		int* val = cf_malloc(sizeof(int*));
		*val = i;
		as_queue_mt_push(&v, &val);
	}
		
	assert(as_queue_mt_size(&v) == 11);
	assert(v.queue.capacity == 20);
	assert(v.queue.flags == 1);
	
	int* result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_mt_pop(&v, &result, AS_QUEUE_NOWAIT)) {
			assert(*result == i);
		}
		else {
			assert(false);
		}
		cf_free(result);
	}

    as_queue_mt_destroy(&v);
}

TEST(types_queue_mt_pop_tail, "as_queue_mt pop tail" ) {
	as_queue_mt v;
	as_queue_mt_init(&v, sizeof(int), 10);

	for (int i = 0; i < 11; i++) {
		as_queue_mt_push(&v, &i);
	}

	assert(as_queue_mt_size(&v) == 11);
	assert(v.queue.capacity == 20);
	assert(v.queue.flags == 1);

	int result;
	for (int i = 10; i >= 0; i--) {
		if (as_queue_mt_pop_tail(&v, &result, AS_QUEUE_NOWAIT)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}

	as_queue_mt_destroy(&v);
}

static as_queue_mt shared_queue;
static int max = 100;
static int count = 0;

static void*
worker1(void* data)
{
	int val = -1;

	while (count < max && as_queue_mt_pop(&shared_queue, &val, AS_QUEUE_FOREVER)) {
		count++;
	}
	return NULL;
}

static void*
worker2(void* data)
{
	for (int i = 0; i < max; i++) {
		as_queue_mt_push(&shared_queue, &i);
	}
	return NULL;
}

TEST(types_queue_mt_thread, "types_queue_mt_thread")
{
	as_queue_mt_init(&shared_queue, sizeof(int), max);

	pthread_t thread1;
	pthread_create(&thread1, NULL, worker1, NULL);

	pthread_t thread2;
	pthread_create(&thread2, NULL, worker2, NULL);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	assert(count == max);

	as_queue_mt_destroy(&shared_queue);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE(types_queue_mt, "as_queue_mt") {
    suite_add(types_queue_mt_stack);
    suite_add(types_queue_mt_heap_init);
    suite_add(types_queue_mt_heap_create);
    suite_add(types_queue_mt_pointers);
	suite_add(types_queue_mt_pop_tail);
	suite_add(types_queue_mt_thread);
}
