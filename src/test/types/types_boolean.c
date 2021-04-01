#include "../test.h"

#include <aerospike/as_boolean.h>
#include <aerospike/as_buffer.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_serializer.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_true, "as_true is true" ) {
	assert( as_boolean_get(&as_true) == true );
}

TEST( types_false, "as_false is false" ) {
	assert( as_boolean_get(&as_false) == false );
}

TEST( types_boolean_true, "as_boolean is true (init)" ) {
	as_boolean b;
	as_boolean_init(&b, true);
	assert( as_boolean_get(&b) == true );
	as_boolean_destroy(&b);
}

TEST( types_boolean_false, "as_boolean is false (init)" ) {
	as_boolean b;
	as_boolean_init(&b, false);
	assert( as_boolean_get(&b) == false );
	as_boolean_destroy(&b);
}

TEST( types_boolean_true_new, "as_boolean is true (new)" ) {
	as_boolean * b = as_boolean_new(true);
	assert( as_boolean_get(b) == true );
	as_boolean_destroy(b);
}

TEST( types_boolean_false_new, "as_boolean is false (new)" ) {
	as_boolean * b = as_boolean_new(false);
	assert( as_boolean_get(b) == false );
	as_boolean_destroy(b);
}

TEST( types_boolean_true_msgpack, "as_boolean is true w/ msgpack" ) {

    as_serializer ser;
    as_msgpack_init(&ser);

    as_buffer buf;
    as_buffer_init(&buf);

    as_serializer_serialize(&ser, (as_val *) &as_true, &buf);

	as_val * v = NULL;
    as_serializer_deserialize(&ser, &buf, &v);

    assert_not_null(v);

    as_boolean * b = as_boolean_fromval(v);
    assert_not_null(b);
	assert(b->value == true);

	as_boolean_destroy(b);
	as_buffer_destroy(&buf);
}

TEST( types_boolean_false_msgpack, "as_boolean is false w/ msgpack" ) {

    as_serializer ser;
    as_msgpack_init(&ser);

    as_buffer buf;
    as_buffer_init(&buf);

    as_serializer_serialize(&ser, (as_val *) &as_false, &buf);

	as_val * v = NULL;
    as_serializer_deserialize(&ser, &buf, &v);

    assert_not_null(v);

    as_boolean * b = as_boolean_fromval(v);
    assert_not_null(b);
	assert(b->value == false);

	as_boolean_destroy(b);
	as_buffer_destroy(&buf);
}

TEST( types_boolean_convert, "convert as_val to bool" )
{
	bool b = as_val_tobool(NULL);
    assert_false(b);

	as_boolean asb;
	as_boolean_init(&asb, false);
 	b = as_val_tobool((as_val*)&asb);
	assert_false(b);
	as_boolean_init(&asb, true);
 	b = as_val_tobool((as_val*)&asb);
    assert_true(b);

	as_integer asi;
	as_integer_init(&asi, 0);
 	b = as_val_tobool((as_val*)&asi);
	assert_false(b);
	as_integer_init(&asi, 1);
 	b = as_val_tobool((as_val*)&asi);
    assert_true(b);
	as_integer_init(&asi, -999);
 	b = as_val_tobool((as_val*)&asi);
    assert_true(b);

	// Conversions from other types should always return false.
	as_double asd;
	as_double_init(&asd, 1.0);
 	b = as_val_tobool((as_val*)&asd);
	assert_false(b);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_boolean, "as_boolean" ) {
	suite_add( types_true );
	suite_add( types_false );
	suite_add( types_boolean_true );
	suite_add( types_boolean_false );
	suite_add( types_boolean_true_new );
	suite_add( types_boolean_false_new );
	suite_add( types_boolean_true_msgpack );
	suite_add( types_boolean_false_msgpack );
	suite_add( types_boolean_convert );
}
