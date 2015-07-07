#include "../test.h"

#include <citrusleaf/alloc.h>

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

	int rc = 0;

	as_val * a = (as_val *) as_string_new_strdup("a");
	as_val * b = (as_val *) as_string_new_strdup("b");
	as_val * c = (as_val *) as_string_new_strdup("c");

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

	// remove an entry

	rc = as_hashmap_remove(m, a);

	assert_int_eq( as_hashmap_size(m), 2 );

	v = (as_integer *) as_hashmap_get(m, a);
	assert_null( v );

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

	int rc = 0;

	as_val * a = (as_val *) as_string_new_strdup("a");
	as_val * b = (as_val *) as_string_new_strdup("b");
	as_val * c = (as_val *) as_string_new_strdup("c");

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

	// remove an entry

	rc = as_map_remove(m, a);

	assert_int_eq( as_map_size(m), 2 );

	v = (as_integer *) as_map_get(m, a);
	assert_null( v );

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

	as_hashmap_set(m, (as_val *) as_string_new_strdup("R"), (as_val *) as_integer_new(1));
	as_hashmap_set(m, (as_val *) as_string_new_strdup("a"), (as_val *) as_integer_new(2));
	as_hashmap_set(m, (as_val *) as_string_new_strdup("N"), (as_val *) as_integer_new(3));
	as_hashmap_set(m, (as_val *) as_string_new_strdup("d"), (as_val *) as_integer_new(4));
	as_hashmap_set(m, (as_val *) as_string_new_strdup("o"), (as_val *) as_integer_new(5));
	as_hashmap_set(m, (as_val *) as_string_new_strdup("M"), (as_val *) as_integer_new(6));
	assert_int_eq( as_map_size((as_map *) m), 6 );

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
		(*sum) = (*sum) + (int)as_integer_get(i);
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
	as_buffer_destroy(&b);
}

TEST( types_hashmap_types, "as_hashmap types" ) {

	as_map * m = (as_map *) as_hashmap_new(5);	
	assert_int_eq( as_map_size(m), 0 );

	// Setting a map
	as_map *map_data = (as_map *)as_hashmap_new(1);
	as_map_set(map_data, (as_val *)as_integer_new(10), (as_val *)as_integer_new(20));
	as_map_set(m, (as_val *) as_string_new_strdup("map_data"), (as_val *) map_data);
	
	// Setting a list
	as_list *list_data = (as_list *) as_arraylist_new(1, 1);
	as_arraylist_set_int64((as_arraylist *)list_data, 0, 20000);
	as_map_set(m, (as_val *) as_string_new_strdup("list_data"), (as_val *) list_data);
	
	// Setting a string
	as_map_set(m, (as_val *) as_string_new_strdup("string_data"), (as_val *) as_string_new_strdup("my string data"));
	
	// Setting an integer
	as_map_set(m, (as_val *) as_string_new_strdup("integer_data"), (as_val *) as_integer_new(10000));

	// Setting a double
	as_map_set(m, (as_val *) as_string_new_strdup("double_data"), (as_val *) as_double_new(1013.4454));

	// Setting a blob
	as_bytes *byte_data = as_bytes_new(20);
	int i = 0; 
	for (i = 0; i<20; i++) {
		as_bytes_set_byte(byte_data,i,i);
	}
	as_map_set(m, (as_val *) as_string_new_strdup("byte_data"), (as_val *) byte_data);

	assert_int_eq( as_map_size(m), 6 );

	// check individual values
	as_string key;
	as_val    *val;
	
	// check map_data
	as_string_init(&key,"map_data",false);
	val = as_map_get(m, (as_val *)&key);
	assert_not_null(val);
	
	as_integer subkey;
	as_integer_init(&subkey, 10);
	as_val 	*subval = as_map_get((as_map *)val, (as_val *)&subkey);
	assert_int_eq (as_integer_get((as_integer *)subval), 20 );

	// check list_data
	as_string_init(&key,"list_data",false);
	val = as_map_get(m, (as_val *)&key);
	assert_not_null(val);
	assert_int_eq (as_list_size((as_list *)val), 1);
	assert_int_eq (as_list_get_int64((as_list *)val, 0), 20000);
	
	// check string data
	as_string_init(&key,"string_data",false);
	val = as_map_get(m, (as_val *)&key);
	assert_not_null(val);
	assert_string_eq (as_string_get((as_string *)val), "my string data");

	// check integer data
	as_string_init(&key,"integer_data",false);
	val = as_map_get(m, (as_val *)&key);
	assert_not_null(val);
	assert_int_eq (as_integer_get((as_integer *)val), 10000);
	
	// check double data
	as_string_init(&key,"double_data",false);
	val = as_map_get(m, (as_val *)&key);
	assert_not_null(val);
	assert_double_eq (as_double_get((as_double *)val), 1013.4454);

	// check bytes data
	as_string_init(&key,"byte_data",false);
	val = as_map_get(m, (as_val *)&key);
	assert_not_null(val);
	assert_int_eq (as_bytes_size((as_bytes *)val), 20);
	uint8_t *bytes = (uint8_t *)as_bytes_get((as_bytes *)val);
	
	for (i=0; i<20; i++) {
		assert_int_eq (*bytes++,i);
	}
	
	as_map_destroy(m);

}


/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_hashmap, "as_hashmap" ) {
	suite_add( types_hashmap_empty );
	suite_add( types_hashmap_ops );
	suite_add( types_hashmap_types );	
	suite_add( types_hashmap_map_ops );
	suite_add( types_hashmap_iterator );
	suite_add( types_hashmap_foreach );
	suite_add( types_hashmap_msgpack );
}
