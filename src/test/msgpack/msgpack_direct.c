#include "../test.h"
#include "../test_common.h"

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_hashmap.h>
#include <aerospike/as_list.h>
#include <aerospike/as_list_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_serializer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_stringmap.h>

#define MAX_BUF_SIZE	2000000

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static as_val *random_val();

static msgpack_compare_t compare_vals(as_val *v1, as_val *v2)
{
	as_serializer ser;
	as_msgpack_init(&ser);

	uint32_t size1 = as_serializer_serialize_getsize(&ser, v1);
	uint32_t size2 = as_serializer_serialize_getsize(&ser, v2);

	unsigned char buf1[size1];
	unsigned char buf2[size2];

	as_serializer_serialize_presized(&ser, v1, buf1);
	as_serializer_serialize_presized(&ser, v2, buf2);

	as_serializer_destroy(&ser);

	as_unpacker pk1 = {
			.buffer = buf1,
			.offset = 0,
			.length = size1,
	};
	as_unpacker pk2 = {
			.buffer = buf2,
			.offset = 0,
			.length = size2,
	};

	msgpack_compare_t ret = as_unpack_compare(&pk1, &pk2);

	if (pk1.offset != pk1.length) {
		return MSGPACK_COMPARE_ERROR;
	}

	if (pk2.offset != pk2.length) {
		return MSGPACK_COMPARE_ERROR;
	}

	return ret;
}

static as_val *random_nil()
{
	return (as_val *)&as_nil;
}

static as_val *random_bool()
{
	return (as_val *)(rand() % 2 == 0 ? &as_false : &as_true);
}

static as_val *random_integer()
{
	return (as_val *)as_integer_new(rand());
}

static as_val *random_string()
{
	int len = rand() % 200;
	char str[len + 1];

	for (int i = 0; i < len; i++) {
		str[i] = rand() % 256;
	}
	str[len] = '\0';

	return (as_val *)as_string_new_strdup(str);
}

static as_val *random_list()
{
	int cap = rand() % 4 + 1;
	as_arraylist *list = as_arraylist_new(cap, cap);

	for (int i = 0; i < cap; i++) {
		as_arraylist_append(list, random_val());
	}

	return (as_val *)list;
}

static as_val *random_map()
{
	int cap = rand() % 3 + 1;
	as_hashmap *val = as_hashmap_new(cap);

	for (int i = 0; i < cap; i++) {
		as_hashmap_set(val, random_val(), random_val());
	}

	return (as_val *)val;
}

typedef as_val *(*random_val_func)();

static as_val *random_val()
{
	static const random_val_func list[] = {
			random_nil,
			random_bool,
			random_integer,
			random_string,
			random_list,
			random_map,
	};
	const size_t size = sizeof(list) / sizeof(random_val_func);
	int r = rand() % (int)size;
	int i = 2;
	while (i-- > 0) {
		// reroll to reduce chance for map/list
		if (r > 3) {
			r = rand() % (int)size;
		}
	}
	return list[r]();
}

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( msgpack_size, "size deep list" )
{
	unsigned char* buf = malloc(MAX_BUF_SIZE);

	memset(buf, 0x91, MAX_BUF_SIZE - 1);
	buf[MAX_BUF_SIZE - 1] = 0x90;

	as_unpacker pk = {
			.length = MAX_BUF_SIZE,
			.buffer = buf,
			.offset = 0
	};

	int64_t size = as_unpack_size(&pk);
	free(buf);

	assert( size == MAX_BUF_SIZE );
}

TEST( msgpack_compare, "compare deep list" )
{
	unsigned char* buf = malloc(MAX_BUF_SIZE);

	memset(buf, 0x91, MAX_BUF_SIZE - 1);
	buf[MAX_BUF_SIZE - 1] = 0x90;

	msgpack_compare_t ret = as_unpack_buf_compare(buf, MAX_BUF_SIZE, buf, MAX_BUF_SIZE);
	free(buf);

	assert( ret == MSGPACK_COMPARE_EQUAL );
}

TEST( msgpack_compare_utf8, "compare utf8" )
{
	as_string s1;
	as_string s2;

	as_string_init(&s1, "hi", false);
	as_string_init(&s2, "hi", false);

	assert( compare_vals((as_val *)&s1, (as_val *)&s2) == MSGPACK_COMPARE_EQUAL );

	as_string_destroy(&s1);
	as_string_destroy(&s2);

	// smiley face emoji is U+1F603
	as_string_init(&s1, "hi😃😁😂😄😅😆", false);
	// neutral face emoji is U+1F610
	as_string_init(&s2, "hi😐😁😂😄😅😆", false);

	assert( compare_vals((as_val *)&s1, (as_val *)&s2) == MSGPACK_COMPARE_LESS );

	as_string_destroy(&s1);
	as_string_destroy(&s2);

	as_string_init(&s1, "hi😃😐😁😂😄😅😆", false);
	as_string_init(&s2, "hi😃😐😁😂😄😅😆", false);

	assert( compare_vals((as_val *)&s1, (as_val *)&s2) == MSGPACK_COMPARE_EQUAL );

	as_string_destroy(&s1);
	as_string_destroy(&s2);

	as_string_init(&s1, "hi😃😐😁😂😄😅😆hello", false);
	as_string_init(&s2, "hi😃😐😁😂😄😅😆", false);

	assert( compare_vals((as_val *)&s1, (as_val *)&s2) == MSGPACK_COMPARE_GREATER );

	as_string_destroy(&s1);
	as_string_destroy(&s2);
}

TEST( msgpack_compare_mixed, "compare mixed" )
{
	as_arraylist list1;
	as_arraylist list2;

	as_arraylist_init(&list1, 10, 10);
	as_arraylist_init(&list2, 10, 10);

	for (int i = 0; i < 9; i++) {
		as_val *val = random_val();
		as_val_reserve(val);

		as_arraylist_append(&list1, val);
		as_arraylist_append(&list2, val);
	}

	assert( compare_vals((as_val *)&list1, (as_val *)&list2) == MSGPACK_COMPARE_EQUAL );

	as_arraylist_append(&list1, (as_val *)as_string_new_strdup("hi😃😐😁😂😄😅😆hello"));
	as_arraylist_append(&list2, (as_val *)as_string_new_strdup("hi😃😐😁😂😄😅😆"));

	assert( compare_vals((as_val *)&list1, (as_val *)&list2) == MSGPACK_COMPARE_GREATER );

	as_arraylist_destroy(&list1);
	as_arraylist_destroy(&list2);
}

TEST( msgpack_compare_mixed_deep, "compare mixed deep" )
{
	as_arraylist list1;
	as_arraylist list2;

	as_arraylist_init(&list1, 10, 10);
	as_arraylist_init(&list2, 10, 10);

	for (int i = 0; i < 9; i++) {
		as_val *val = random_val();
		as_val_reserve(val);

		as_arraylist_append(&list1, val);
		as_arraylist_append(&list2, val);
	}

	as_arraylist list1_1;
	as_arraylist list2_1;

	as_arraylist_init(&list1_1, 10, 10);
	as_arraylist_init(&list2_1, 10, 10);

	as_arraylist_insert(&list1, 7, (as_val *)&list1_1);
	as_arraylist_insert(&list2, 7, (as_val *)&list2_1);

	for (int i = 0; i < 9; i++) {
		as_val *val = random_val();
		as_val_reserve(val);

		as_arraylist_append(&list1_1, val);
		as_arraylist_append(&list2_1, val);
	}

	assert( compare_vals((as_val *)&list1, (as_val *)&list2) == MSGPACK_COMPARE_EQUAL );

	as_arraylist_insert(&list1_1, 7, (as_val *)as_string_new_strdup("hi😃😐😁😂😄😅😆hello"));
	as_arraylist_insert(&list2_1, 7, (as_val *)as_string_new_strdup("hi😃😐😁😂😄😅😆"));

	assert( compare_vals((as_val *)&list1, (as_val *)&list2) == MSGPACK_COMPARE_GREATER );

	as_arraylist_append(&list1, (as_val *)&as_nil);
	as_arraylist_append(&list1, (as_val *)&as_nil);
	as_arraylist_append(&list2_1, (as_val *)&as_nil);
	as_arraylist_append(&list2_1, (as_val *)&as_nil);

	assert( compare_vals((as_val *)&list1, (as_val *)&list2) == MSGPACK_COMPARE_GREATER );

	as_arraylist_destroy(&list1);
	as_arraylist_destroy(&list2);
}

TEST( msgpack_int_direct, "signed int direct" )
{
	const int64_t range = 1000000;
	uint8_t buf[(sizeof(uint64_t) + 1) * 10];

	for (int64_t i = -range; i < range; i += 10) {
		as_packer pk = {
				.buffer = buf,
				.capacity = (int)sizeof(buf)
		};

		for (int j = 0; j < 10; j++) {
			as_pack_int64(&pk, i + j);
		}

		as_unpacker upk = {
				.buffer = buf,
				.length = pk.offset
		};

		for (int j = 0; j < 10; j++) {
			int64_t result = 0;

			as_unpack_int64(&upk, &result);
			assert( i + j == result );
		}
	}
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( msgpack_direct, "as_msgpack direct size/compare" ) {
	suite_add( msgpack_size );
	suite_add( msgpack_compare );
	suite_add( msgpack_compare_utf8 );
	suite_add( msgpack_compare_mixed );
	suite_add( msgpack_compare_mixed_deep );
	suite_add( msgpack_int_direct );
}
