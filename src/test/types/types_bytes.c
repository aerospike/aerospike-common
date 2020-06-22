#include "../test.h"

#include <aerospike/as_bytes.h>
#include <string.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST(types_bytes_null, "as_bytes containing NULL")
{
    as_bytes b;
    as_bytes_init(&b, 0);
    assert(as_bytes_size(&b) == 0);
    as_bytes_destroy(&b);
}

TEST(types_bytes_empty, "as_bytes containing \"\"")
{
    as_bytes b;
    as_bytes_init_wrap(&b, (uint8_t *) "", 0, false);
    assert(as_bytes_size(&b) == 0);
    as_bytes_destroy(&b);
}

TEST(types_bytes_random, "as_bytes containing a value")
{
	uint8_t test_str[] = "dskghseoighweg";
    uint32_t test_len = sizeof(test_str);

    as_bytes b;
    as_bytes_init_wrap(&b, test_str, test_len,false);

    assert(as_bytes_size(&b) == test_len);
    
    as_bytes_destroy(&b);
}

TEST(types_bytes_get_set, "as_bytes getting and setting")
{
	uint8_t test_literal[] = "dskghseoighweg";
    uint32_t test_len = sizeof(test_literal);  // includes null terminated byte.
    
    // Do not wrap literal because that is considered read-only.
    // Instead, wrap stack variable.
    uint8_t* test_str = alloca(test_len);
	memcpy(test_str, test_literal, test_len);
				
    uint8_t v;

    as_bytes b;
    as_bytes_init_wrap(&b, test_str, test_len, false);
    
    assert(as_bytes_get(&b) == b.value);
    assert(as_bytes_size(&b) == test_len);

    assert_false(as_bytes_set(&b, test_len+2, &v, 1)); // should fail out of bounds
    assert_false(as_bytes_copy(&b, test_len+2, &v, 1)); // should fail out of bounds
    assert_false(as_bytes_copy(&b, 0, &v, test_len+2)); // should fail out of bounds
    assert_false(as_bytes_copy(&b, test_len/2, &v, test_len)); // should fail out of bounds

    // test a basic get
    assert(1 == as_bytes_copy(&b, 3, &v, 1));
    assert(v == test_str[3]);

    // then a set
    v = 0x17;
    as_bytes_set(&b, 4, &v, 1);

    v = 0;
    assert(1 == as_bytes_copy(&b, 4, &v, 1));
    assert(v == 0x17);

    // test the end of the range
    v = 0xff;
    assert_true(as_bytes_set(&b, test_len-1, &v, 1));

    v = 0;
    assert_true(as_bytes_copy(&b, test_len-1, &v, 1));
    assert(v == 0xff);

    // reset - which is a range set
    assert_true(as_bytes_set(&b, 0, test_literal, test_len));

    // test a longer range for get
    uint8_t v3[3];
    
    as_bytes_copy(&b, 4 /*pos*/, v3, sizeof(v3));
    assert(0 == memcmp(v3, &test_str[4], sizeof(v)));

    as_bytes_copy(&b, 7 /*pos*/, v3, sizeof(v3));
    assert(0 == memcmp(v3, &test_str[7], sizeof(v)));

    as_bytes_destroy(&b);
}

TEST(types_bytes_stack_append, "as_bytes append on stack")
{
	as_bytes b;
	as_bytes_inita(&b, 27);

	assert_int_eq(b.size, 0);
	assert_int_eq(b.capacity, 27);

	for (int i = 0; i < 26; i++) {
		as_bytes_append_byte(&b, 'a' + i);
	}
	as_bytes_append_byte(&b, 0);

	assert_int_eq(b.size, 27);

	char str[27] = {0};
	uint32_t len = as_bytes_copy(&b, 0, (uint8_t *) str, sizeof(str));

	assert_int_eq(len, 27);

	assert_string_eq(str, "abcdefghijklmnopqrstuvwxyz");
	assert_string_eq((char *) b.value, "abcdefghijklmnopqrstuvwxyz");

    as_bytes_destroy(&b);
}

TEST(types_bytes_stack_append_set, "as_bytes append on stack, then set values")
{
	as_bytes b;
	as_bytes_inita(&b, 27);

	assert_int_eq(b.size, 0);
	assert_int_eq(b.capacity, 27);

	for (int i = 0; i < 26; i++) {
		as_bytes_append_byte(&b, 'a' + i);
	}
	as_bytes_append_byte(&b, 0);

	assert_int_eq(b.size, 27);

	char str[27] = {0};
	uint32_t len = as_bytes_copy(&b, 0, (uint8_t *) str, sizeof(str));

	assert_int_eq(len, 27);

	assert_string_eq(str, "abcdefghijklmnopqrstuvwxyz");
	assert_string_eq((char *) b.value, "abcdefghijklmnopqrstuvwxyz");
	
	for (int i = 0; i < 26; i++) {
		if (i % 4 == 0)
		as_bytes_set_byte(&b, i, '$');
	}

	assert_string_eq((char *) b.value, "$bcd$fgh$jkl$nop$rst$vwx$z");

    as_bytes_destroy(&b);
}

TEST(types_bytes_hex, "byte/hex string conversions")
{
	// Test insufficient string size.
	uint8_t bytes[] = {(uint8_t)1, (uint8_t)11, (uint8_t)254};
	char s1[6];
	bool rv = as_bytes_to_string(bytes, sizeof(bytes), s1, sizeof(s1));
	assert_false(rv);
	assert_string_eq(s1, "");

	// Test sufficient string size.
	char s2[7];
	rv = as_bytes_to_string(bytes, sizeof(bytes), s2, sizeof(s2));
	assert_true(rv);
	assert_string_eq(s2, "010BFE");

	// Test insufficient prefix string size.
	char s3[8];
	rv = as_bytes_to_string_with_prefix(bytes, sizeof(bytes), s3, sizeof(s3));
	assert_false(rv);
	assert_string_eq(s3, "");

	// Test sufficient prefix string size.
	char s4[9];
	rv = as_bytes_to_string_with_prefix(bytes, sizeof(bytes), s4, sizeof(s4));
	assert_true(rv);
	assert_string_eq(s4, "0x010BFE");

	// Test uneven hex string.
	uint8_t* b2 = alloca(3);
	int len = as_bytes_from_string(b2, 3, "12345");
	assert_int_eq(len, -1);

	// Test insufficient byte size.
	len = as_bytes_from_string(b2, 3, "123456FF");
	assert_int_eq(len, -1);

	// Test sufficient byte size.
	uint8_t* b3 = alloca(4);
	len = as_bytes_from_string(b3, 4, "123456Ff");
	assert_int_eq(len, 4);
	uint8_t expect3[] = {(uint8_t)18, (uint8_t)52, (uint8_t)86, (uint8_t)255};
    assert(memcmp(b3, expect3, sizeof(expect3)) == 0);

	// Test invalid hex char.
	len = as_bytes_from_string(b3, 4, "123G56Ff");
	assert_int_eq(len, -1);

	// Test prefix.
	len = as_bytes_from_string(b3, 4, s4);
	assert_int_eq(len, 3);
    assert(memcmp(b3, bytes, sizeof(bytes)) == 0);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE(types_bytes, "as_bytes")
{
    suite_add(types_bytes_null);
    suite_add(types_bytes_empty);
    suite_add(types_bytes_random);
    suite_add(types_bytes_get_set);
    suite_add(types_bytes_stack_append);
    suite_add(types_bytes_stack_append_set);
    suite_add(types_bytes_hex);
}
