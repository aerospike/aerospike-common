#include "../test.h"

#include <aerospike/as_bytes.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_bytes_null, "as_bytes containing NULL" ) {
    as_bytes b;
    as_bytes_init(&b, 0, 0, false);
    assert( as_bytes_len(&b) == 0 );
    as_bytes_destroy(&b);
}

TEST( types_bytes_empty, "as_bytes containing \"\"" ) {
    as_bytes b;
    as_bytes_init(&b,(uint8_t *) "", 0, false);
    assert( as_bytes_len(&b) == 0 );
    as_bytes_destroy(&b);
}

static uint8_t test_str[] = "dskghseoighweg";

TEST( types_bytes_random, "as_bytes containing a value" ) {
    as_bytes b;
    size_t test_len = sizeof(test_str); // include the terminating null as a test
    as_bytes_init(&b,test_str, test_len,false);
    assert( as_bytes_len(&b) == test_len );
    as_bytes_destroy(&b);
}

TEST( types_bytes_get_set, "as_bytes getting and setting" ) {
    as_bytes b;
    uint8_t  v;
    size_t test_len = sizeof(test_str);;
    as_bytes_init(&b,test_str, test_len,false);
    assert( as_bytes_len(&b) == test_len );
    assert( -1 == as_bytes_set(&b, test_len+2, &v, 1) ); // should fail out of bounds
    assert( -1 == as_bytes_get(&b, test_len+2, &v, 1) ); // should fail out of bounds
    assert( -1 == as_bytes_get(&b, 0, &v, test_len+2) ); // should fail out of bounds
    assert( -1 == as_bytes_get(&b, test_len/2, &v, test_len) ); // should fail out of bounds

    // test a basic get
    assert( 0 == as_bytes_get(&b, 3, &v, 1) );
    assert( v == test_str[3]);

    // then a set
    v = 0x17;
    as_bytes_set(&b, 4, &v, 1);
    v = 0;
    assert( 0 == as_bytes_get(&b, 4, &v, 1));
    assert( v == 0x17);

    // test the end of the range
    v = 0xff;
    assert(0 == as_bytes_set(&b, test_len-1, &v, 1));
    v = 0;
    assert(0 == as_bytes_get(&b, test_len-1, &v, 1));
    assert(v == 0xff);

    // reset - which is a range set
    assert(0 == as_bytes_set(&b, 0, test_str, test_len));

    // test a longer range for get
    uint8_t    v3[3];
    as_bytes_get(&b, 4 /*pos*/, v3, sizeof(v3));
    assert( 0 == memcmp(v3, &test_str[4], sizeof(v)));
    as_bytes_get(&b, 7 /*pos*/, v3, sizeof(v3));
    assert( 0 == memcmp(v3, &test_str[7], sizeof(v)));

    as_bytes_destroy(&b);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_bytes, "as_bytes" ) {
    suite_add( types_bytes_null );
    suite_add( types_bytes_empty );
    suite_add( types_bytes_random );
    suite_add( types_bytes_get_set );
}