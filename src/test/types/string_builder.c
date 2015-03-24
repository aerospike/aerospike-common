#include "../test.h"

#include <aerospike/as_string_builder.h>
#include <string.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( string_builder_noresize, "string builder no-resize" ) {
	
	as_string_builder sb;
	as_string_builder_inita(&sb, 10, false);
	
	// Normal append
	bool status = as_string_builder_append(&sb, "abcde");
	assert(status);
	assert(sb.length == 5);
	assert(sb.capacity == 10);
	
	// This append will not be successful because the "j" doesn't fit.
	status = as_string_builder_append(&sb, "fghij");
	assert(!status);
	
	// Ensure more appends continue to fail.
	status = as_string_builder_append(&sb, "x");
	assert(!status);
	
	// Verify expected string.
	assert(strcmp(sb.data, "abcdefghi") == 0);
	assert(sb.length == 9);
	
	as_string_builder_destroy(&sb);
}

TEST( string_builder_resize_stack, "string builder resize stack" ) {
	
	as_string_builder sb;
	as_string_builder_inita(&sb, 10, true);
	
	// Normal append
	bool status = as_string_builder_append(&sb, "abcde");
	assert(status);
	assert(!sb.free);
	
	// This append will force resize to heap.
	status = as_string_builder_append(&sb, "fghij");
	assert(status);
	assert(strcmp(sb.data, "abcdefghij") == 0);
	assert(sb.capacity == 20);
	assert(sb.length == 10);
	assert(sb.free);
	
	// This append should succeed without resize.
	status = as_string_builder_append(&sb, "012345678");
	assert(status);
	assert(strcmp(sb.data, "abcdefghij012345678") == 0);
	assert(sb.capacity == 20);

	// This append will force heap realloc.
	status = as_string_builder_append_char(&sb, 'x');
	assert(status);
	assert(strcmp(sb.data, "abcdefghij012345678x") == 0);
	assert(sb.capacity == 40);
	
	as_string_builder_destroy(&sb);
}

TEST( string_builder_resize_heap, "string builder resize heap" ) {
	
	as_string_builder sb;
	as_string_builder_init(&sb, 10, true);
	
	// Normal append
	bool status = as_string_builder_append(&sb, "abcde");
	assert(status);
	assert(sb.free);
		
	// This append will force heap realloc with long string.
	status = as_string_builder_append(&sb, "01234567890123456789");
	assert(status);
	assert(strcmp(sb.data, "abcde01234567890123456789") == 0);
	assert(sb.length == 25);
	assert(sb.capacity == 26);
	assert(sb.free);
	
	as_string_builder_destroy(&sb);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( string_builder, "string builder" ) {
    suite_add( string_builder_noresize );
    suite_add( string_builder_resize_stack );
    suite_add( string_builder_resize_heap );
}
