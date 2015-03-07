#include "../test.h"

#include <aerospike/as_nil.h>
#include <aerospike/as_string.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_nil_string_conversion, "as_nil's string conversion is a NIL" ) {
    as_val *v = (as_val *)&as_nil;
    char *str = as_nil_val_tostring(v);
	assert_string_eq( str, "NIL" );
	free (str);
}

TEST( types_nil_length_check, "as_nil having zero items" ) {
    as_val *v = (as_val *)&as_nil;
    assert( v->count == 0 );
}

TEST( types_nil_hash_zero, "as_nil always hashes to zero" ) {
    as_val *v = (as_val *)&as_nil;
    assert( as_nil_val_hashcode(v) == 0 );
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_nil, "as_nil" ) {
    suite_add( types_nil_string_conversion );
    suite_add( types_nil_length_check );
    suite_add( types_nil_hash_zero );
}
