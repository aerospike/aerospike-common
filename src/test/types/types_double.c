
#include "../test.h"

#include <float.h>

#include <aerospike/as_double.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_double_0, "as_double containing 0" ) {
    as_double i;
    as_double_init(&i,0);
    assert( as_double_get(&i) == 0 );
}

TEST( types_double_pos_1, "as_double containing 1" ) {
    as_double i;
    as_double_init(&i,1);
    assert( as_double_get(&i) == 1 );
}

TEST( types_double_neg_1, "as_double containing -1" ) {
    as_double i;
    as_double_init(&i,-1);
    assert( as_double_get(&i) == -1 );
}

TEST( types_double_max, "as_double containing DBL_MAX" ) {
    as_double i;
    as_double_init(&i,DBL_MAX);
    assert( as_double_get(&i) == DBL_MAX );
}

TEST( types_double_min, "as_double containing DBL_MIN" ) {
    as_double i;
    as_double_init(&i,DBL_MIN);
    assert( as_double_get(&i) == DBL_MIN );
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_double, "as_double" ) {
    suite_add( types_double_0 );
    suite_add( types_double_pos_1 );
    suite_add( types_double_neg_1 );
    suite_add( types_double_max );
    suite_add( types_double_min );
}
