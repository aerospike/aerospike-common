#include "../test.h"

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_map.h>
#include <aerospike/as_pair.h>
#include <aerospike/as_string.h>
#include <aerospike/as_stringmap.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_serializer.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_hashmap_empty, "as_hashmap is empty" ) {
	as_hashmap * m = as_hashmap_new(0);
	assert_int_eq( as_map_size((as_map *) m), 0 );
	as_map_destroy((as_map *) m);
}

TEST( types_hashmap_ops, "as_hashmap ops" ) {

	as_val * a = (as_val *) as_string_new(strdup("a"), true);
	as_val * b = (as_val *) as_string_new(strdup("b"), true);
	as_val * c = (as_val *) as_string_new(strdup("c"), true);

	as_integer * v = NULL;

	as_hashmap * m = as_hashmap_new(10);
	assert_int_eq( as_hashmap_size(m), 0 );

	as_hashmap_set(m, as_val_reserve(a), (as_val *) as_integer_new(1));
	as_hashmap_set(m, as_val_reserve(b), (as_val *) as_integer_new(2));
	as_hashmap_set(m, as_val_reserve(c), (as_val *) as_integer_new(3));

	assert_int_eq( as_hashmap_size(m), 3 );

	// check individual values

	v = (as_integer *) as_hashmap_get(m, a);
	assert_int_eq( v->value, 1 );

	v = (as_integer *) as_hashmap_get(m, b);
	assert_int_eq( v->value, 2 );

	v = (as_integer *) as_hashmap_get(m, c);
	assert_int_eq( v->value, 3 );

	// Resetting the values
	
	as_hashmap_set(m, as_val_reserve(a), (as_val *) as_integer_new(4));
	as_hashmap_set(m, as_val_reserve(b), (as_val *) as_integer_new(5));
	as_hashmap_set(m, as_val_reserve(c), (as_val *) as_integer_new(6));

	assert_int_eq( as_hashmap_size(m), 3 );

	// check individual values

	v = (as_integer *) as_hashmap_get(m, a);
	assert_int_eq( v->value, 4 );

	v = (as_integer *) as_hashmap_get(m, b);
	assert_int_eq( v->value, 5 );

	v = (as_integer *) as_hashmap_get(m, c);
	assert_int_eq( v->value, 6 );

	// Clear the map

	as_hashmap_clear(m);
	assert_int_eq( as_hashmap_size(m), 0 );

	// Resetting the values

	as_hashmap_set(m, as_val_reserve(a), (as_val *) as_integer_new(7));
	as_hashmap_set(m, as_val_reserve(b), (as_val *) as_integer_new(8));
	as_hashmap_set(m, as_val_reserve(c), (as_val *) as_integer_new(9));

	assert_int_eq( as_hashmap_size(m), 3 );


	v = (as_integer *) as_hashmap_get(m, a);
	assert_int_eq( v->value, 7 );

	v = (as_integer *) as_hashmap_get(m, b);
	assert_int_eq( v->value, 8 );

	v = (as_integer *) as_hashmap_get(m, c);
	assert_int_eq( v->value, 9 );

	as_hashmap_destroy(m);

	as_val_destroy(a);
	as_val_destroy(b);
	as_val_destroy(c);
}

TEST( types_hashmap_map_ops, "as_hashmap w/ as_map ops" ) {

	as_val * a = (as_val *) as_string_new(strdup("a"), true);
	as_val * b = (as_val *) as_string_new(strdup("b"), true);
	as_val * c = (as_val *) as_string_new(strdup("c"), true);

	as_integer * v = NULL;

	as_map * m = (as_map *) as_hashmap_new(10);
	
	assert_int_eq( as_map_size(m), 0 );

	// Setting the values

	as_map_set(m, as_val_reserve(a), (as_val *) as_integer_new(1));
	as_map_set(m, as_val_reserve(b), (as_val *) as_integer_new(2));
	as_map_set(m, as_val_reserve(c), (as_val *) as_integer_new(3));

	assert_int_eq( as_map_size(m), 3 );

	// check individual values

	v = (as_integer *) as_map_get(m, a);
	assert_int_eq( v->value, 1 );

	v = (as_integer *) as_map_get(m, b);
	assert_int_eq( v->value, 2 );

	v = (as_integer *) as_map_get(m, c);
	assert_int_eq( v->value, 3 );

	// Resetting the values
	
	as_map_set(m, as_val_reserve(a), (as_val *) as_integer_new(4));
	as_map_set(m, as_val_reserve(b), (as_val *) as_integer_new(5));
	as_map_set(m, as_val_reserve(c), (as_val *) as_integer_new(6));

	assert_int_eq( as_map_size(m), 3 );

	// check individual values

	v = (as_integer *) as_map_get(m, a);
	assert_int_eq( v->value, 4 );

	v = (as_integer *) as_map_get(m, b);
	assert_int_eq( v->value, 5 );

	v = (as_integer *) as_map_get(m, c);
	assert_int_eq( v->value, 6 );

	// Clear the map

	as_map_clear(m);
	assert_int_eq( as_map_size(m), 0 );

	// Resetting the values

	as_map_set(m, as_val_reserve(a), (as_val *) as_integer_new(7));
	as_map_set(m, as_val_reserve(b), (as_val *) as_integer_new(8));
	as_map_set(m, as_val_reserve(c), (as_val *) as_integer_new(9));

	assert_int_eq( as_map_size(m), 3 );

	v = (as_integer *) as_map_get(m, a);
	assert_int_eq( v->value, 7 );

	v = (as_integer *) as_map_get(m, b);
	assert_int_eq( v->value, 8 );

	v = (as_integer *) as_map_get(m, c);
	assert_int_eq( v->value, 9 );

	as_map_destroy((as_map *) m);

	as_val_destroy(a);
	as_val_destroy(b);
	as_val_destroy(c);
}


TEST( types_hashmap_iterator, "as_hashmap w/ as_iterator ops" ) {

	as_hashmap * m = as_hashmap_new(10);
	assert_int_eq( as_map_size((as_map *) m), 0 );

	as_hashmap_set(m, (as_val *) as_string_new(strdup("a"), true), (as_val *) as_integer_new(1));
	as_hashmap_set(m, (as_val *) as_string_new(strdup("b"), true), (as_val *) as_integer_new(2));
	as_hashmap_set(m, (as_val *) as_string_new(strdup("c"), true), (as_val *) as_integer_new(3));
	assert_int_eq( as_map_size((as_map *) m), 3 );

	as_iterator * i = (as_iterator *) as_hashmap_iterator_new(m);

	int count = 0;
	while ( as_iterator_has_next(i) ) {
		as_pair * p = (as_pair *) as_iterator_next(i);
		as_integer * a = (as_integer *) as_pair_2(p);
		as_integer * e = (as_integer *) as_map_get((as_map *) m, as_pair_1(p));
		assert( a->value == e->value );
		count ++;
	}

	as_iterator_destroy(i);

	assert_int_eq( as_hashmap_size(m), count );

	as_hashmap_destroy(m);
}


bool types_hashmap_foreach_callback(const as_val * key, const as_val * val, void * udata)
{
	int * sum = (int *) udata;
	as_integer * i = as_integer_fromval(val);
	if ( i ) {
		(*sum) = (*sum) + as_integer_get(i);
	}
	return true;
}

TEST( types_hashmap_foreach, "as_hashmap_foreach" ) {

	int sum = 0;

	as_hashmap * m = as_hashmap_new(10);
	as_stringmap_set_int64((as_map *) m, "a", 1);
	as_stringmap_set_int64((as_map *) m, "b", 2);
	as_stringmap_set_int64((as_map *) m, "c", 3);

	as_hashmap_foreach(m, types_hashmap_foreach_callback, &sum);

	assert_int_eq( sum, 6 );

	as_hashmap_destroy(m);
}


TEST( types_hashmap_msgpack, "as_hashmap msgpack" ) {

	as_hashmap * m1 = as_hashmap_new(10);
	as_stringmap_set_int64((as_map *) m1, "a", 1);
	as_stringmap_set_int64((as_map *) m1, "b", 2);
	as_stringmap_set_int64((as_map *) m1, "c", 3);

	assert_int_eq( as_hashmap_size(m1), 3 );
	assert_int_eq( as_stringmap_get_int64((as_map *) m1, "a"), 1);
	assert_int_eq( as_stringmap_get_int64((as_map *) m1, "b"), 2);
	assert_int_eq( as_stringmap_get_int64((as_map *) m1, "c"), 3);
	
    as_serializer ser;
    as_msgpack_init(&ser);

    as_buffer b;
    as_buffer_init(&b);

    as_serializer_serialize(&ser, (as_val *) m1, &b);

	as_val * v2 = NULL;
    as_serializer_deserialize(&ser, &b, &v2);

    assert_not_null( v2 );
    assert_int_eq( as_val_type(v2), AS_MAP );

    as_map * m2 = as_map_fromval(v2);
	assert_int_eq( as_map_size(m2), as_hashmap_size(m1) );

	assert_int_eq( as_stringmap_get_int64(m2, "a"), 1);
	assert_int_eq( as_stringmap_get_int64(m2, "b"), 2);
	assert_int_eq( as_stringmap_get_int64(m2, "c"), 3);
	
	as_map_destroy(m2);
	as_hashmap_destroy(m1);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_hashmap, "as_hashmap" ) {
	suite_add( types_hashmap_empty );
	suite_add( types_hashmap_ops );
	suite_add( types_hashmap_map_ops );
	suite_add( types_hashmap_iterator );
	suite_add( types_hashmap_foreach );
	suite_add( types_hashmap_msgpack );
}
