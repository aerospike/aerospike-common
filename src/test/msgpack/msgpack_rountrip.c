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

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static as_val * roundtrip(as_val * in)
{
	as_val * out = NULL;

	as_serializer ser;
	as_msgpack_init(&ser);

	as_buffer b;
	as_buffer_init(&b);

	as_serializer_serialize(&ser, in, &b);
	as_serializer_deserialize(&ser, &b, &out);
	as_buffer_destroy(&b);

	return out;
}


/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( msgpack_roundtrip_integer1, "roundtrip: 123" )
{
	as_integer i1;
	as_integer_init(&i1, 123);

	as_val * v2 = roundtrip((as_val *) &i1);

	assert_val_eq(v2, &i1);

	as_integer_destroy(&i1);
	as_val_destroy(v2);
}

TEST( msgpack_roundtrip_double1, "roundtrip: 123.456" )
{
	as_double d1;
	as_double_init(&d1, 123.456);
	
	as_val * v2 = roundtrip((as_val *) &d1);
	
	assert_val_eq(v2, &d1);
	
	as_double_destroy(&d1);
	as_val_destroy(v2);
}

TEST( msgpack_roundtrip_string1, "roundtrip: \"abc\"" )
{
	as_string s1;
	as_string_init(&s1, "abc", false);

	as_val * v2 = roundtrip((as_val *) &s1);

	assert_val_eq(v2, &s1);

	as_string_destroy(&s1);
	as_val_destroy(v2);
}

TEST( msgpack_roundtrip_list1, "roundtrip: [123,456,789]" )
{
	as_arraylist l1;
	as_arraylist_inita(&l1,3);
	as_arraylist_append_int64(&l1, 123);
	as_arraylist_append_int64(&l1, 456);
	as_arraylist_append_int64(&l1, 789);

	as_val * v2 = roundtrip((as_val *) &l1);

	assert_val_eq(v2, &l1);

	as_arraylist_destroy(&l1);
	as_val_destroy(v2);
}

TEST( msgpack_roundtrip_list2, "roundtrip: [{'a': 1, 'b': 2, 'c': 3}, {'d': 4, 'e': 5, 'f': 6}, {'g': 7, 'h': 8, 'i': 9}]" )
{
	as_hashmap m1;
	as_hashmap_init(&m1,3);
	as_stringmap_set_int64((as_map *) &m1, "a", 1);
	as_stringmap_set_int64((as_map *) &m1, "b", 2);
	as_stringmap_set_int64((as_map *) &m1, "c", 3);

	as_hashmap m2;
	as_hashmap_init(&m2,3);
	as_stringmap_set_int64((as_map *) &m2, "d", 4);
	as_stringmap_set_int64((as_map *) &m2, "e", 5);
	as_stringmap_set_int64((as_map *) &m2, "f", 6);

	as_hashmap m3;
	as_hashmap_init(&m3,3);
	as_stringmap_set_int64((as_map *) &m3, "g", 7);
	as_stringmap_set_int64((as_map *) &m3, "h", 8);
	as_stringmap_set_int64((as_map *) &m3, "i", 9);

	as_arraylist l1;
	as_arraylist_inita(&l1,3);
	as_arraylist_append_map(&l1, (as_map *) &m1);
	as_arraylist_append_map(&l1, (as_map *) &m2);
	as_arraylist_append_map(&l1, (as_map *) &m3);
	
	as_val * v2 = roundtrip((as_val *) &l1);

	assert_val_eq(v2, &l1);

	as_arraylist_destroy(&l1);
	as_val_destroy(v2);
}

TEST( msgpack_roundtrip_map1, "roundtrip: {'abc': 123, 'def': 456, 'ghi': 789}" )
{
	as_hashmap m1;
	as_hashmap_init(&m1,3);
	as_stringmap_set_int64((as_map *) &m1, "abc", 123);
	as_stringmap_set_int64((as_map *) &m1, "def", 456);
	as_stringmap_set_int64((as_map *) &m1, "ghi", 789);

	as_val * v2 = roundtrip((as_val *) &m1);

	assert_val_eq(v2, &m1);

	as_hashmap_destroy(&m1);
	as_val_destroy(v2);
}

TEST( msgpack_roundtrip_map2, "roundtrip: {'abc': [1,2,3], 'def': [4,5,6], 'ghi': [7,8,9]}" )
{
	as_arraylist l1;
	as_arraylist_inita(&l1,3);
	as_arraylist_append_int64(&l1, 1);
	as_arraylist_append_int64(&l1, 2);
	as_arraylist_append_int64(&l1, 3);
	
	as_arraylist l2;
	as_arraylist_inita(&l2,3);
	as_arraylist_append_int64(&l2, 4);
	as_arraylist_append_int64(&l2, 5);
	as_arraylist_append_int64(&l2, 6);

	as_arraylist l3;
	as_arraylist_inita(&l3,3);
	as_arraylist_append_int64(&l3, 7);
	as_arraylist_append_int64(&l3, 8);
	as_arraylist_append_int64(&l3, 9);

	as_hashmap m1;
	as_hashmap_init(&m1,3);
	as_stringmap_set_list((as_map *) &m1, "abc", (as_list *) &l1);
	as_stringmap_set_list((as_map *) &m1, "def", (as_list *) &l2);
	as_stringmap_set_list((as_map *) &m1, "ghi", (as_list *) &l3);

	as_val * v2 = roundtrip((as_val *) &m1);

	assert_val_eq(v2, &m1);

	as_hashmap_destroy(&m1);
	as_val_destroy(v2);
}
/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( msgpack_roundtrip, "as_msgpack roundtrip serialize/deserialize" ) {
	suite_add( msgpack_roundtrip_integer1 );
	suite_add( msgpack_roundtrip_double1 );
	suite_add( msgpack_roundtrip_string1 );
	suite_add( msgpack_roundtrip_list1 );
	suite_add( msgpack_roundtrip_list2 );
	suite_add( msgpack_roundtrip_map1 );
	suite_add( msgpack_roundtrip_map2 );
}
