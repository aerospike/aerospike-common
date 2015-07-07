#include "../test.h"

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>
#include <aerospike/as_list_iterator.h>
#include <aerospike/as_msgpack.h>
#include <aerospike/as_serializer.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_arraylist_empty, "as_arraylist is empty" ) {
    as_arraylist l;
    as_arraylist_init(&l,0,0);
    assert( as_list_size((as_list *) &l) == 0 );
    as_list_destroy((as_list *) &l);
}

#if 0 // DEBUG
static void print_list(const char *msg, const as_list *l) {
	char *s = as_val_tostring(l);
	fprintf(stderr, "%s %s\n",msg,s);
	cf_free(s);
}
#endif

TEST( types_arraylist_cap10_blk0, "as_arraylist w/ capacity 10, block_size 0" ) {

    int rc = 0;

    as_arraylist l;
    as_arraylist_init(&l,10,0);

    assert_int_eq( l.capacity, 10 );
    assert_int_eq( l.block_size, 0 );
    assert_int_eq( l.size, 0 );

    assert( l.size == as_list_size((as_list *) &l) );
    assert_int_eq( l.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_arraylist_append_integer(&l, as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_arraylist_size(&l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }

    for ( int i = 6; i < 11; i++) {
        rc = as_arraylist_append_integer(&l, as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_arraylist_size(&l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }

    as_integer * i11 = as_integer_new(11);

    rc = as_arraylist_append_integer(&l, i11);
    assert_int_ne( rc, AS_ARRAYLIST_OK );
    assert( l.size == as_arraylist_size(&l) );
    assert_int_eq( l.size, 10);
    assert_int_eq( l.capacity, 10);

    as_integer * i12 = as_integer_new(12);

    rc = as_arraylist_append_integer(&l, i12);
    assert_int_ne( rc, AS_ARRAYLIST_OK );
    assert( l.size == as_arraylist_size(&l) );
    assert_int_eq( l.size, 10);
    assert_int_eq( l.capacity, 10);
    
    as_arraylist_destroy(&l);
    as_integer_destroy(i11);
    as_integer_destroy(i12);
}

TEST( types_arraylist_cap10_blk10, "as_arraylist w/ capacity 10, block_size 10" ) {

    int rc = 0;

    as_arraylist l;
    as_arraylist_init(&l,10,10);

    assert_int_eq( l.capacity, 10 );
    assert_int_eq( l.block_size, 10 );
    assert_int_eq( l.size, 0 );

    assert( l.size == as_list_size((as_list *) &l) );
    assert_int_eq( l.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_arraylist_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_list_size((as_list *) &l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }

    for ( int i = 6; i < 11; i++) {
        rc = as_arraylist_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_list_size((as_list *) &l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }

    rc = as_arraylist_append(&l, (as_val *) as_integer_new(11));
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert( l.size == as_list_size((as_list *) &l) );
    assert_int_eq( l.size, 11);
    assert_int_eq( l.capacity, 20);

    rc = as_arraylist_append(&l, (as_val *) as_integer_new(12));
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert( l.size == as_list_size((as_list *) &l));
    assert_int_eq( l.size, 12);
    assert_int_eq( l.capacity, 20);
    
    as_arraylist_destroy(&l);

}

TEST( types_arraylist_1, "as_arraylist w/ as_arraylist ops" ) {

    int rc = 0;

    as_arraylist l;
    as_arraylist_init((as_arraylist *) &l,10,10);

    assert_int_eq( l.capacity, 10 );
    assert_int_eq( l.block_size, 10 );
    assert_int_eq( l.size, 0 );

    assert( l.size == as_arraylist_size(&l) );
    assert_int_eq( l.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_arraylist_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_arraylist_size(&l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }
    // list is now: 1 2 3 4 5

    for ( int i = 6; i < 11; i++) {
        rc = as_arraylist_prepend(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_arraylist_size(&l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }
    // list is now: 10 9 8 7 6 1 2 3 4 5
    // indexes:      0 1 2 3 4 5 6 7 8 9

    assert_int_eq(9, as_integer_toint((as_integer *) as_arraylist_get(&l, 1)));
    assert_int_eq(2, as_integer_toint((as_integer *) as_arraylist_get(&l, 6)));
    assert(NULL == as_arraylist_get(&l,10)); // off end is safe
    	
    as_arraylist * t = as_arraylist_take(&l, 5); // take first five
    assert_int_eq( as_arraylist_size(t), 5 );

    assert_int_eq(10, as_integer_toint((as_integer *) as_arraylist_get(t,0)));
    assert_int_eq(6, as_integer_toint((as_integer *) as_arraylist_get(t,4)));

    as_integer * t_head = (as_integer *) as_arraylist_head(t);
    as_integer * l_head = (as_integer *) as_arraylist_head(&l);

    assert( as_integer_toint(t_head) == as_integer_toint(l_head) );
    
    as_arraylist_destroy(t);
    t = 0;

    as_arraylist * d = as_arraylist_drop(&l, 5); // drop returns list from 5 forward, does not change l
    assert_int_eq( as_arraylist_size(d), 5 );
    assert_int_eq( as_arraylist_size(&l), 10);

    assert_int_eq(2, as_integer_toint((as_integer *) as_arraylist_get(d,1)));

    as_integer * d_0 = (as_integer *) as_arraylist_get(d, 0); // should be 1
    as_integer * l_5 = (as_integer *) as_arraylist_get(&l, 5); // 5 from 

    assert( as_integer_toint(d_0) == as_integer_toint(l_5) );

    as_arraylist_destroy(d);

    rc = as_arraylist_insert(&l, 5, (as_val *) as_integer_new(99));
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 11 );
    assert_int_eq(6, as_integer_toint((as_integer *) as_arraylist_get(&l, 4)));
    assert_int_eq(99, as_integer_toint((as_integer *) as_arraylist_get(&l, 5)));
    assert_int_eq(1, as_integer_toint((as_integer *) as_arraylist_get(&l, 6)));
    assert_int_eq(5, as_integer_toint((as_integer *) as_arraylist_get(&l, 10)));
    // list is now: 10 9 8 7 6 99 1 2 3 4  5
    // indexes:      0 1 2 3 4  5 6 7 8 9 10

    rc = as_arraylist_remove(&l, 11);
    assert_int_eq( rc, AS_ARRAYLIST_ERR_INDEX );
    rc = as_arraylist_remove(&l, 4);
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 10 );
    assert_int_eq(7, as_integer_toint((as_integer *) as_arraylist_get(&l, 3)));
    assert_int_eq(99, as_integer_toint((as_integer *) as_arraylist_get(&l, 4)));
    assert_int_eq(1, as_integer_toint((as_integer *) as_arraylist_get(&l, 5)));
    assert_int_eq(5, as_integer_toint((as_integer *) as_arraylist_get(&l, 9)));
    // list is now: 10 9 8 7 99 1 2 3 4 5
    // indexes:      0 1 2 3  4 5 6 7 8 9

    as_arraylist c;
    as_arraylist_init((as_arraylist *) &c, 10, 10);

    for ( int i = 1; i < 4; i++) {
        rc = as_arraylist_append(&c, (as_val *) as_integer_new(100 + i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
    }

    assert_int_eq( c.size, 3 );
    rc = as_arraylist_concat(&l, &c);
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 13 );
    assert_int_eq(5, as_integer_toint((as_integer *) as_arraylist_get(&l, 9)));
    assert_int_eq(101, as_integer_toint((as_integer *) as_arraylist_get(&l, 10)));
    assert_int_eq(103, as_integer_toint((as_integer *) as_arraylist_get(&l, 12)));
    // list is now: 10 9 8 7 99 1 2 3 4 5 101 102 103
    // indexes:      0 1 2 3  4 5 6 7 8 9  10  11  12

    as_arraylist_destroy(&c);

    rc = as_arraylist_trim(&l, 13);
    assert_int_eq( rc, AS_ARRAYLIST_ERR_INDEX );
    rc = as_arraylist_trim(&l, 7);
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 7 );
    assert_int_eq(2, as_integer_toint((as_integer *) as_arraylist_get(&l, 6)));
    // list is now: 10 9 8 7 99 1 2
    // indexes:      0 1 2 3  4 5 6

    as_arraylist_destroy(&l);
}


TEST( types_arraylist_list, "as_arraylist w/ as_list ops" ) {

    int rc = 0;

    as_arraylist l;
    as_arraylist_init((as_arraylist *) &l,10,10);

    assert_int_eq( l.capacity, 10 );
    assert_int_eq( l.block_size, 10 );
    assert_int_eq( l.size, 0 );

    assert( l.size == as_list_size((as_list *) &l) );
    assert_int_eq( l.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_list_append((as_list *) &l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_list_size((as_list *) &l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }
    // list is now: 1 2 3 4 5

    for ( int i = 6; i < 11; i++) {
        rc = as_list_prepend((as_list *) &l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.size == as_list_size((as_list *) &l) );
        assert_int_eq( l.size, i );
        assert_int_eq( l.capacity, 10);
    }
    // list is now: 10 9 8 7 6 1 2 3 4 5
    // indexes:      0 1 2 3 4 5 6 7 8 9

    assert_int_eq(9, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 1)));
    assert_int_eq(2, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 6)));
    assert(NULL == as_list_get((as_list *) &l,10)); // off end is safe
    	
    as_list * t = as_list_take((as_list *) &l, 5); // take first five
    assert_int_eq( as_list_size((as_list *) t), 5 );

    assert_int_eq(10, as_integer_toint((as_integer *) as_list_get((as_list *) t,0)));
    assert_int_eq(6, as_integer_toint((as_integer *) as_list_get((as_list *) t,4)));

    as_integer * t_head = (as_integer *) as_list_head((as_list *) t);
    as_integer * l_head = (as_integer *) as_list_head((as_list *) &l);

    assert( as_integer_toint(t_head) == as_integer_toint(l_head) );
    
    as_list_destroy((as_list *) t);
    t = 0;

    as_list * d = as_list_drop((as_list *) &l, 5); // drop returns list from 5 forward, does not change l
    assert_int_eq( as_list_size((as_list *) d), 5 );
    assert_int_eq( as_list_size((as_list *) &l), 10);

    assert_int_eq(2, as_integer_toint((as_integer *) as_list_get((as_list *) d,1)));

    as_integer * d_0 = (as_integer *) as_list_get((as_list *) d, 0); // should be 1
    as_integer * l_5 = (as_integer *) as_list_get((as_list *) &l, 5); // 5 from 

    assert( as_integer_toint(d_0) == as_integer_toint(l_5) );

    as_list_destroy((as_list *) d);

    rc = as_list_insert((as_list *) &l, 5, (as_val *) as_integer_new(99));
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 11 );
    assert_int_eq(6, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 4)));
    assert_int_eq(99, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 5)));
    assert_int_eq(1, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 6)));
    assert_int_eq(5, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 10)));
    // list is now: 10 9 8 7 6 99 1 2 3 4  5
    // indexes:      0 1 2 3 4  5 6 7 8 9 10

    rc = as_list_remove((as_list *) &l, 11);
    assert_int_eq( rc, AS_ARRAYLIST_ERR_INDEX );
    rc = as_list_remove((as_list *) &l, 4);
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 10 );
    assert_int_eq(7, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 3)));
    assert_int_eq(99, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 4)));
    assert_int_eq(1, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 5)));
    assert_int_eq(5, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 9)));
    // list is now: 10 9 8 7 99 1 2 3 4 5
    // indexes:      0 1 2 3  4 5 6 7 8 9

    as_arraylist c;
    as_arraylist_init(&c, 10, 10);

    for ( int i = 1; i < 4; i++) {
        rc = as_list_append((as_list *) &c, (as_val *) as_integer_new(100 + i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
    }

    assert_int_eq( c.size, 3 );
    rc = as_list_concat((as_list *) &l, (const as_list *) &c);
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 13 );
    assert_int_eq(5, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 9)));
    assert_int_eq(101, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 10)));
    assert_int_eq(103, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 12)));
    // list is now: 10 9 8 7 99 1 2 3 4 5 101 102 103
    // indexes:      0 1 2 3  4 5 6 7 8 9  10  11  12

    as_arraylist_destroy(&c);

    rc = as_list_trim((as_list *) &l, 13);
    assert_int_eq( rc, AS_ARRAYLIST_ERR_INDEX );
    rc = as_list_trim((as_list *) &l, 7);
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert_int_eq( l.size, 7 );
    assert_int_eq(2, as_integer_toint((as_integer *) as_list_get((as_list *) &l, 6)));
    // list is now: 10 9 8 7 99 1 2
    // indexes:      0 1 2 3  4 5 6

    as_list_destroy((as_list *) &l);
}

TEST( types_arraylist_iterator, "as_arraylist w/ as_iterator ops" ) {

    as_arraylist l;
    as_arraylist_init(&l,10,10);
    
    for ( int i = 1; i < 6; i++) {
        as_list_append((as_list *) &l, (as_val *) as_integer_new(i));
    }

    assert_int_eq( as_list_size((as_list *) &l), 5 );

    as_iterator * i = (as_iterator *) as_arraylist_iterator_new(&l);

    as_integer * v = NULL;

    assert_true( as_iterator_has_next(i) );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 1 );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 2 );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 3 );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 4 );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 5 );

    assert_false( as_iterator_has_next(i) );

    as_iterator_destroy(i);

    as_arraylist_destroy(&l);
}

TEST( types_arraylist_msgpack, "as_arraylist msgpack" ) {

    as_arraylist l1;
    as_arraylist_init(&l1,10,0);
    as_arraylist_append_int64(&l1, 123);
    as_arraylist_append_int64(&l1, 456);
    as_arraylist_append_int64(&l1, 789);
    as_arraylist_append_double(&l1, 92.1);
    as_arraylist_append_double(&l1, -23.596);
    as_arraylist_append_str(&l1, "abc");
    as_arraylist_append_str(&l1, "def");
    as_arraylist_append_str(&l1, "ghi");
    
    assert_int_eq( as_arraylist_size(&l1), 8 );

    as_serializer ser;
    as_msgpack_init(&ser);

    as_buffer b;
    as_buffer_init(&b);

    as_serializer_serialize(&ser, (as_val *) &l1, &b);

    as_val * v2 = NULL;

    as_serializer_deserialize(&ser, &b, &v2);

    assert_not_null( v2 );
    assert_int_eq( as_val_type(v2), AS_LIST );

    as_list * l2 = as_list_fromval(v2);

    assert_int_eq( as_arraylist_size(&l1), as_list_size(l2) );
    assert_int_eq( as_arraylist_get_int64(&l1, 0), 123 );
    assert_int_eq( as_arraylist_get_int64(&l1, 1), 456 );
    assert_int_eq( as_arraylist_get_int64(&l1, 2), 789 );
    assert_double_eq( as_arraylist_get_double(&l1, 3), 92.1 );
    assert_double_eq( as_arraylist_get_double(&l1, 4), -23.596 );
    assert_string_eq( as_arraylist_get_str(&l1, 5), "abc" );
    assert_string_eq( as_arraylist_get_str(&l1, 6), "def" );
    assert_string_eq( as_arraylist_get_str(&l1, 7), "ghi" );

    as_arraylist_destroy(&l1);
    
    as_list_destroy(l2);
    
    as_buffer_destroy(&b);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_arraylist, "as_arraylist" ) {
    suite_add( types_arraylist_empty );
    suite_add( types_arraylist_cap10_blk0 );
    suite_add( types_arraylist_cap10_blk10 );
    suite_add( types_arraylist_1 );
    suite_add( types_arraylist_list );
    suite_add( types_arraylist_iterator );
    suite_add( types_arraylist_msgpack );
}
