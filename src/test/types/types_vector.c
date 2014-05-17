#include "../test.h"

#include <aerospike/as_vector.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_vector_stack, "as_vector stack mode" ) {
	// Initialize vector on stack.
    as_vector v;
	as_vector_inita(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_vector_append(&v, &i);
	}
	
	assert(v.size == 9);
	assert(v.capacity == 10);
	assert(v.flags == 0);
	
	for (int i = 0; i < 9; i++) {
		int* result = as_vector_get(&v, i);
		assert(*result == i);
	}
	
	// Add more items so vector is resized/converted to heap.
	int i = 9;
	as_vector_append(&v, &i);
	i = 10;
	as_vector_append(&v, &i);
	
	assert(v.size == 11);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	for (int i = 0; i < 11; i++) {
		int* result = as_vector_get(&v, i);
		assert(*result == i);
	}
	
	// Destroy vector.
    as_vector_destroy(&v);
}

TEST( types_vector_heap_init, "as_vector heap with init" ) {
	// Initialize array on heap.
    as_vector v;
	as_vector_init(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_vector_append(&v, &i);
	}
	
	assert(v.size == 9);
	assert(v.capacity == 10);
	assert(v.flags == 1);
	
	for (int i = 0; i < 9; i++) {
		int* result = as_vector_get(&v, i);
		assert(*result == i);
	}
	
	// Add more items so vector is resized.
	int i = 9;
	as_vector_append(&v, &i);
	i = 10;
	as_vector_append(&v, &i);
	
	assert(v.size == 11);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	for (int i = 0; i < 11; i++) {
		int* result = as_vector_get(&v, i);
		assert(*result == i);
	}
	
	// Destroy vector.
    as_vector_destroy(&v);
}

TEST( types_vector_heap_create, "as_vector heap with create" ) {
	// Initialize array on heap.
    as_vector* v = as_vector_create(sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_vector_append(v, &i);
	}
	
	assert(v->size == 9);
	assert(v->capacity == 10);
	assert(v->flags == 3);
	
	for (int i = 0; i < 9; i++) {
		int* result = as_vector_get(v, i);
		assert(*result == i);
	}
	
	// Add more items so vector is resized.
	int i = 9;
	as_vector_append(v, &i);
	i = 10;
	as_vector_append(v, &i);
	
	assert(v->size == 11);
	assert(v->capacity == 20);
	assert(v->flags == 3);
	
	for (int i = 0; i < 11; i++) {
		int* result = as_vector_get(v, i);
		assert(*result == i);
	}
	
	// Destroy vector.
    as_vector_destroy(v);
}

TEST( types_vector_append_unique, "as_vector append unique" ) {
    as_vector v;
	as_vector_init(&v, sizeof(int), 10);
	
	for (int i = 0; i < 9; i++) {
		as_vector_append(&v, &i);
	}
	
	// Should succeed.
	int i = 9;
	as_vector_append_unique(&v, &i);
	
	// Should fail.
	i = 5;
	as_vector_append_unique(&v, &i);
	
	assert(v.size == 10);
	assert(v.capacity == 10);
	assert(v.flags == 1);
		
    as_vector_destroy(&v);
}

TEST( types_vector_pointers, "as_vector pointer elements" ) {
    as_vector v;
	as_vector_init(&v, sizeof(int*), 10);
	
	for (int i = 0; i < 11; i++) {
		int* val = cf_malloc(sizeof(int*));
		*val = i;
		as_vector_append(&v, &val);
	}
		
	assert(v.size == 11);
	assert(v.capacity == 20);
	assert(v.flags == 1);
	
	for (int i = 0; i < 11; i++) {
		int* result = as_vector_get_ptr(&v, i);
		assert(*result == i);
		cf_free(result);
	}

    as_vector_destroy(&v);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_vector, "as_vector" ) {
    suite_add( types_vector_stack );
    suite_add( types_vector_heap_init );
    suite_add( types_vector_heap_create );
    suite_add( types_vector_append_unique );
    suite_add( types_vector_pointers );
}
