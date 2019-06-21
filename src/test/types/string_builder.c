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
	status = as_string_builder_append(&sb, "01234567");
	assert(status);
	assert(strcmp(sb.data, "abcdefghij01234567") == 0);
	assert(sb.capacity == 20);

	// This append should succeed without resize.
	status = as_string_builder_append_char(&sb, '8');
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

TEST( string_builder_bytes, "string builder append bytes" ) {
	
	as_string_builder sb;
	as_string_builder_inita(&sb, 11, false);
	
	// Normal append
	uint8_t b1[] = {0x11, 0x22, 0x33};
	bool status = as_string_builder_append_bytes(&sb, b1, sizeof(b1));
	assert(status);
	assert(sb.length == 10);
	assert(sb.capacity == 11);
	
	// This append will not be successful because extra byte doesn't fit.
	uint8_t b2[] = {0x44};
	status = as_string_builder_append_bytes(&sb, b2, sizeof(b2));
	assert(!status);

	// Verify expected string.
	assert(sb.length == 10);
	assert(strcmp(sb.data, "[11 22 33]") == 0);

	as_string_builder_destroy(&sb);
}

TEST( string_builder_bytes_resize, "string builder append bytes with resize" ) {
	
	as_string_builder sb;
	as_string_builder_inita(&sb, 12, true);
	
	// Normal append
	uint8_t b1[] = {0x11, 0x22, 0x33, 0x44};
	bool status = as_string_builder_append_bytes(&sb, b1, sizeof(b1));
	assert(status);
	assert(sb.length == 13);
	assert(sb.capacity == 24);
	
	// Verify expected string.
	assert(sb.length == 13);
	assert(strcmp(sb.data, "[11 22 33 44]") == 0);

	as_string_builder_destroy(&sb);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( string_builder, "string builder" ) {
    suite_add( string_builder_noresize );
    suite_add( string_builder_resize_stack );
    suite_add( string_builder_resize_heap );
    suite_add( string_builder_bytes );
    suite_add( string_builder_bytes_resize );
}
