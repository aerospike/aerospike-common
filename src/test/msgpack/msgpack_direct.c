#include "../test.h"
#include "../test_common.h"

#include <stdlib.h>
#include <string.h>

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_hashmap.h>
#include <aerospike/as_list.h>
#include <aerospike/as_list_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_msgpack_ext.h>
#include <aerospike/as_serializer.h>
#include <aerospike/as_string.h>
#include <aerospike/as_stringmap.h>

#define MAX_BUF_SIZE	2000000

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static as_val *random_val();

static msgpack_compare_t
compare_vals(as_val *v1, as_val *v2)
{
	as_serializer ser;
	as_msgpack_init(&ser);

	uint32_t size1 = as_serializer_serialize_getsize(&ser, v1);
	uint32_t size2 = as_serializer_serialize_getsize(&ser, v2);

	uint8_t *buf1 = alloca(size1);
	uint8_t *buf2 = alloca(size2);

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
		info("pk1.offset(%u) != pk1.length(%u)", pk1.offset, pk1.length)
		return MSGPACK_COMPARE_ERROR;
	}

	if (pk2.offset != pk2.length) {
		info("pk2.offset(%u) != pk2.length(%u)", pk2.offset, pk2.length)
		return MSGPACK_COMPARE_ERROR;
	}

	return ret;
}

static msgpack_compare_t
compare_bufs(uint8_t *buf1, uint32_t size1, uint8_t *buf2, uint32_t size2)
{
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
		info("pk1.offset(%u) != pk1.length(%u)", pk1.offset, pk1.length)
		return MSGPACK_COMPARE_ERROR;
	}

	if (pk2.offset != pk2.length) {
		info("pk2.offset(%u) != pk2.length(%u)", pk2.offset, pk2.length)
		return MSGPACK_COMPARE_ERROR;
	}

	return ret;
}

static as_val *
random_nil()
{
	return (as_val *)&as_nil;
}

static as_val *
random_bool()
{
	return (as_val *)(rand() % 2 == 0 ? &as_false : &as_true);
}

static as_val *
random_integer()
{
	return (as_val *)as_integer_new(rand());
}

static as_val *
random_string()
{
	int len = rand() % 200;
	char* str = alloca(len + 1);

	for (int i = 0; i < len; i++) {
		str[i] = rand() % 256;
	}
	str[len] = '\0';

	return (as_val *)as_string_new_strdup(str);
}

static as_val *
random_list()
{
	int cap = rand() % 4 + 1;
	as_arraylist *list = as_arraylist_new(cap, cap);

	for (int i = 0; i < cap; i++) {
		as_arraylist_append(list, random_val());
	}

	return (as_val *)list;
}

static as_val *
random_map()
{
	int cap = rand() % 3 + 1;
	as_hashmap *val = as_hashmap_new(cap);

	for (int i = 0; i < cap; i++) {
		as_hashmap_set(val, random_val(), random_val());
	}

	return (as_val *)val;
}

typedef as_val *(*random_val_func)();

static as_val *
random_val()
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
	// uint8_t buf[MAX_BUF_SIZE] will cause stack overflow on windows.
	// Use malloc instead.
	uint8_t* buf = malloc(MAX_BUF_SIZE);

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
	// uint8_t buf[MAX_BUF_SIZE] will cause stack overflow on windows.
	// Use malloc instead.
	uint8_t* buf = malloc(MAX_BUF_SIZE);

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

TEST( msgpack_compare_mixed_level2, "compare mixed level 2" )
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

TEST( msgpack_compare_lists, "compare lists" )
{
	as_arraylist list0;
	int lens[] = {2, 5, 3, 4};
	const char *items[] = {"item1", "item2"};

	as_arraylist_init(&list0, 10, 10);

	for (int i = 0; i < 10; i++) {
		as_arraylist *listp = as_arraylist_new(5, 5);

		as_arraylist_append_str(listp, items[i % 2]);

		for (int j = 1; j < lens[i % 4]; j++) {
			as_arraylist_append_int64(listp, j);
		}

		{
			char *s = as_val_tostring((as_val *)listp);
			info("list[%d]: %s", i, s);
			free(s);
		}

		as_arraylist_append(&list0, (as_val *)listp);
	}


	as_arraylist cmp1;
	as_arraylist_init(&cmp1, 10, 10);
	as_arraylist_append_str(&cmp1, "item1");
	as_arraylist_append(&cmp1, (as_val *)&as_cmp_wildcard);

	{
		char *s = as_val_tostring((as_val *)&cmp1);
		info("cmp1: %s", s);
		free(s);
	}

	uint32_t eq_count = 0;
	for (int i = 0; i < 10; i++) {
		as_list *cmp0 = as_arraylist_get_list(&list0, i);
		msgpack_compare_t cmp = compare_vals((as_val *)cmp0, (as_val *)&cmp1);

		assert_false(cmp == MSGPACK_COMPARE_ERROR);

		if (cmp == MSGPACK_COMPARE_EQUAL) {
			eq_count++;
			msgpack_compare_t cmp2 = compare_vals((as_val *)&cmp1, (as_val *)cmp0);
			assert_true(cmp2 == MSGPACK_COMPARE_EQUAL);
		}
	}
	assert_int_eq(eq_count, 5);

	as_arraylist_destroy(&cmp1);

	as_arraylist list1;
	as_arraylist_init(&list1, 10, 10);
	as_arraylist_concat(&list1, &list0);

	assert_int_eq(compare_vals((as_val *)&list0, (as_val *)&list1), MSGPACK_COMPARE_EQUAL);

	as_arraylist_append_int64(&list1, 1);

	assert_int_eq(compare_vals((as_val *)&list0, (as_val *)&list1), MSGPACK_COMPARE_LESS);

	as_arraylist_remove(&list1, as_arraylist_size(&list1) - 1);

	as_arraylist_destroy(&list0);
	as_arraylist_destroy(&list1);
}

TEST( msgpack_int_direct, "signed int direct" )
{
	const int64_t range = 1000000;
	uint8_t buf[(sizeof(uint64_t) + 1) * 10];

	for (int64_t i = -range; i < range; i += 10) {
		as_packer pk = {
			.buffer = buf,
			.capacity = (uint32_t)sizeof(buf)
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

TEST( msgpack_deep, "deep list/map" )
{
	// uint8_t buf[MAX_BUF_SIZE] will cause stack overflow on windows.
	// Use malloc instead.
	uint8_t* buf0 = malloc(MAX_BUF_SIZE);
	uint8_t* buf1 = malloc(MAX_BUF_SIZE);

	as_packer pk0 = {
			.buffer = buf0,
			.capacity = MAX_BUF_SIZE
	};

	as_packer pk1 = {
			.buffer = buf1,
			.capacity = MAX_BUF_SIZE
	};

	for (int i = 0; i < 300; i++) {
		as_pack_list_header(&pk0, 1);
		as_pack_list_header(&pk1, 1);
	}

	as_packer save0 = pk0;
	as_packer save1 = pk1;
	msgpack_compare_t cmp;
	const uint32_t shift = 299;

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 1);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_LESS);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_LESS);

	pk0 = save0;
	pk1 = save1;

	as_pack_int64(&pk0, 1);
	as_pack_int64(&pk1, 1);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);

	pk0 = save0;
	pk1 = save1;

	as_pack_list_header(&pk0, 2);
	as_pack_list_header(&pk1, 2);

	as_pack_list_header(&pk0, 2);
	as_pack_list_header(&pk1, 2);

	save0 = pk0;
	save1 = pk1;

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 1);

	as_pack_int64(&pk0, 2);
	as_pack_int64(&pk1, 2);

	as_pack_int64(&pk0, 3);
	as_pack_int64(&pk1, 3);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_LESS);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_LESS);

	pk0 = save0;
	pk1 = save1;

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 0);

	as_pack_int64(&pk0, 2);
	as_pack_int64(&pk1, 2);

	as_pack_int64(&pk0, 3);
	as_pack_int64(&pk1, 3);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);

	pk0 = save0;
	pk1 = save1;

	as_pack_int64(&pk0, 0);
	as_pack_cmp_wildcard(&pk1);

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 2);

	as_pack_int64(&pk0, 3);
	as_pack_int64(&pk1, 3);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);

	pk0 = save0;
	pk1 = save1;

	as_pack_list_header(&pk0, 2);
	as_pack_list_header(&pk1, 3);

	save0 = pk0;
	save1 = pk1;

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk0, 1);
	as_pack_int64(&pk1, 0);
	as_pack_int64(&pk1, 1);
	as_pack_int64(&pk1, 1);

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 0);

	as_pack_int64(&pk0, 3);
	as_pack_int64(&pk1, 3);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_LESS);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_LESS);

	pk0 = save0;
	pk1 = save1;

	as_pack_cmp_wildcard(&pk0);
	as_pack_int64(&pk0, 1);
	as_pack_int64(&pk1, 0);
	as_pack_int64(&pk1, 1);
	as_pack_int64(&pk1, 1);

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 0);

	as_pack_int64(&pk0, 3);
	as_pack_int64(&pk1, 3);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_EQUAL);

	pk0 = save0;
	pk1 = save1;

	as_pack_cmp_wildcard(&pk0);
	as_pack_int64(&pk0, 1);
	as_pack_int64(&pk1, 0);
	as_pack_int64(&pk1, 1);
	as_pack_int64(&pk1, 1);

	as_pack_int64(&pk0, 0);
	as_pack_int64(&pk1, 0);

	as_pack_int64(&pk0, 3);
	as_pack_int64(&pk1, 2);

	cmp = compare_bufs(pk0.buffer, pk0.offset, pk1.buffer, pk1.offset);
	assert_int_eq(cmp, MSGPACK_COMPARE_GREATER);
	cmp = compare_bufs(pk0.buffer + shift, pk0.offset - shift,
			pk1.buffer + shift, pk1.offset - shift);
	assert_int_eq(cmp, MSGPACK_COMPARE_GREATER);

	free(buf0);
	free(buf1);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( msgpack_direct, "as_msgpack direct size/compare" )
{
	suite_add( msgpack_size );
	suite_add( msgpack_compare );
	suite_add( msgpack_compare_utf8 );
	suite_add( msgpack_compare_mixed );
	suite_add( msgpack_compare_mixed_level2 );
	suite_add( msgpack_compare_lists );
	suite_add( msgpack_int_direct );
	suite_add( msgpack_deep );
}
