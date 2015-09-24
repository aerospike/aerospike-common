#include "../test.h"

#include <aerospike/as_queue.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_queue_stack, "as_queue stack mode" ) {
	// Initialize queue on stack.
    as_queue v;
	as_queue_inita(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_queue_push(&v, &i);
	}
	
	assert(as_queue_size(&v) == 9);
	assert(v.capacity == 10);
	assert(v.flags == 0);
	
	// Add more items so queue is resized/converted to heap.
	int i = 9;
	as_queue_push(&v, &i);
	i = 10;
	as_queue_push(&v, &i);
	
	assert(as_queue_size(&v) == 11);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	int result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_pop(&v, &result)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}
	
	assert(as_queue_size(&v) == 0);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	// Destroy queue.
    as_queue_destroy(&v);
}

TEST( types_queue_heap_init, "as_queue heap with init" ) {
	// Initialize array on heap.
    as_queue v;
	as_queue_init(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_queue_push(&v, &i);
	}
	
	assert(as_queue_size(&v) == 9);
	assert(v.capacity == 10);
	assert(v.flags == 1);
		
	// Add more items so queue is resized.
	int i = 9;
	as_queue_push(&v, &i);
	i = 10;
	as_queue_push(&v, &i);
	
	assert(as_queue_size(&v) == 11);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	int result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_pop(&v, &result)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}
	
	assert(as_queue_size(&v) == 0);
	assert(v.capacity == 20);
	assert(v.flags == 1);

	// Destroy queue.
    as_queue_destroy(&v);
}

TEST( types_queue_heap_create, "as_queue heap with create" ) {
	// Initialize array on heap.
    as_queue* v = as_queue_create(sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_queue_push(v, &i);
	}
	
	assert(as_queue_size(v) == 9);
	assert(v->capacity == 10);
	assert(v->flags == 3);
		
	// Add more items so queue is resized.
	int i = 9;
	as_queue_push(v, &i);
	i = 10;
	as_queue_push(v, &i);
	
	assert(as_queue_size(v) == 11);
	assert(v->capacity == 20);
	assert(v->flags == 3);
	
	int result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_pop(v, &result)) {
			assert(result == i);
		}
		else {
			assert(false);
		}
	}
	
	assert(as_queue_size(v) == 0);
	assert(v->capacity == 20);
	assert(v->flags == 3);

	// Destroy queue.
    as_queue_destroy(v);
}

TEST( types_queue_pointers, "as_queue pointer elements" ) {
    as_queue v;
	as_queue_init(&v, sizeof(int*), 10);
	
	for (int i = 0; i < 11; i++) {
		int* val = cf_malloc(sizeof(int*));
		*val = i;
		as_queue_push(&v, &val);
	}
		
	assert(as_queue_size(&v) == 11);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	int* result;
	for (int i = 0; i < 11; i++) {
		if (as_queue_pop(&v, &result)) {
			assert(*result == i);
		}
		else {
			assert(false);
		}
		cf_free(result);
	}

    as_queue_destroy(&v);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_queue, "as_queue" ) {
    suite_add( types_queue_stack );
    suite_add( types_queue_heap_init );
    suite_add( types_queue_heap_create );
    suite_add( types_queue_pointers );
}
