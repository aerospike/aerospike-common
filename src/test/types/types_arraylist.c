#include "../test.h"

#include <aerospike/as_arraylist.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_list.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_arraylist_empty, "as_arraylist is empty" ) {
    as_list l;
    as_arraylist_init(&l,0,0);
    assert( as_list_size(&l) == 0 );
    as_list_destroy(&l);
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

    as_list l;
    as_arraylist_init(&l,10,0);

    assert_int_eq( l.data.arraylist.capacity, 10 );
    assert_int_eq( l.data.arraylist.block_size, 0 );
    assert_int_eq( l.data.arraylist.size, 0 );

    // as_list l;
    // as_list_init(&l, &a, &as_arraylist_list);

    assert( l.data.arraylist.size == as_list_size(&l) );
    assert_int_eq( l.data.arraylist.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_list_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.data.arraylist.size == as_list_size(&l) );
        assert_int_eq( l.data.arraylist.size, i );
        assert_int_eq( l.data.arraylist.capacity, 10);
    }

    for ( int i = 6; i < 11; i++) {
        rc = as_list_prepend(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.data.arraylist.size == as_list_size(&l) );
        assert_int_eq( l.data.arraylist.size, i );
        assert_int_eq( l.data.arraylist.capacity, 10);
    }

    rc = as_list_append(&l, (as_val *) as_integer_new(11));
    assert_int_ne( rc, AS_ARRAYLIST_OK );
    assert( l.data.arraylist.size == as_list_size(&l) );
    assert_int_eq( l.data.arraylist.size, 10);
    assert_int_eq( l.data.arraylist.capacity, 10);

    rc = as_list_prepend(&l, (as_val *) as_integer_new(12));
    assert_int_ne( rc, AS_ARRAYLIST_OK );
    assert( l.data.arraylist.size == as_list_size(&l));
    assert_int_eq( l.data.arraylist.size, 10);
    assert_int_eq( l.data.arraylist.capacity, 10);
    
    as_list_destroy(&l);

}

TEST( types_arraylist_cap10_blk10, "as_arraylist w/ capacity 10, block_size 10" ) {

    int rc = 0;

    as_list l;
    as_arraylist_init(&l,10,10);

    assert_int_eq( l.data.arraylist.capacity, 10 );
    assert_int_eq( l.data.arraylist.block_size, 10 );
    assert_int_eq( l.data.arraylist.size, 0 );

    assert( l.data.arraylist.size == as_list_size(&l) );
    assert_int_eq( l.data.arraylist.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_list_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.data.arraylist.size == as_list_size(&l) );
        assert_int_eq( l.data.arraylist.size, i );
        assert_int_eq( l.data.arraylist.capacity, 10);
    }

    for ( int i = 6; i < 11; i++) {
        rc = as_list_prepend(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.data.arraylist.size == as_list_size(&l) );
        assert_int_eq( l.data.arraylist.size, i );
        assert_int_eq( l.data.arraylist.capacity, 10);
    }

    rc = as_list_append(&l, (as_val *) as_integer_new(11));
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert( l.data.arraylist.size == as_list_size(&l) );
    assert_int_eq( l.data.arraylist.size, 11);
    assert_int_eq( l.data.arraylist.capacity, 20);

    rc = as_list_prepend(&l, (as_val *) as_integer_new(12));
    assert_int_eq( rc, AS_ARRAYLIST_OK );
    assert( l.data.arraylist.size == as_list_size(&l));
    assert_int_eq( l.data.arraylist.size, 12);
    assert_int_eq( l.data.arraylist.capacity, 20);
    
    as_list_destroy(&l);

}

TEST( types_arraylist_list, "as_arraylist w/ list ops" ) {

    int rc = 0;

    as_list l;
    as_arraylist_init(&l,10,10);

    assert_int_eq( l.data.arraylist.capacity, 10 );
    assert_int_eq( l.data.arraylist.block_size, 10 );
    assert_int_eq( l.data.arraylist.size, 0 );

    assert( l.data.arraylist.size == as_list_size(&l) );
    assert_int_eq( l.data.arraylist.size, 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_list_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.data.arraylist.size == as_list_size(&l) );
        assert_int_eq( l.data.arraylist.size, i );
        assert_int_eq( l.data.arraylist.capacity, 10);
    }
    // list is now: 1 2 3 4 5

    for ( int i = 6; i < 11; i++) {
        rc = as_list_prepend(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, AS_ARRAYLIST_OK );
        assert( l.data.arraylist.size == as_list_size(&l) );
        assert_int_eq( l.data.arraylist.size, i );
        assert_int_eq( l.data.arraylist.capacity, 10);
    }
    // list is now: 10 9 8 7 6 1 2 3 4 5
    // indexes:      0 1 2 3 4 5 6 7 8 9

    assert_int_eq(9, as_integer_toint((as_integer *) as_list_get(&l, 1)));
    assert_int_eq(2, as_integer_toint((as_integer *) as_list_get(&l, 6)));
    assert(NULL == as_list_get(&l,10)); // off end is safe
    	
    as_list * t = as_list_take(&l, 5); // take first five
    assert_int_eq( as_list_size(t), 5 );

    assert_int_eq(10, as_integer_toint((as_integer *) as_list_get(t,0)));
    assert_int_eq(6, as_integer_toint((as_integer *) as_list_get(t,4)));

    as_integer * t_head = (as_integer *) as_list_head(t);
    as_integer * l_head = (as_integer *) as_list_head(&l);

    assert( as_integer_toint(t_head) == as_integer_toint(l_head) );
    
    as_list_destroy(t);
    t = 0;

    as_list * d = as_list_drop(&l, 5); // drop returns list from 5 forward, does not change l
    assert_int_eq( as_list_size(d), 5 );
    assert_int_eq( as_list_size(&l), 10);

    assert_int_eq(2, as_integer_toint((as_integer *) as_list_get(d,1)));

    as_integer * d_0 = (as_integer *) as_list_get(d, 0); // should be 1
    as_integer * l_5 = (as_integer *) as_list_get(&l, 5); // 5 from 

    assert( as_integer_toint(d_0) == as_integer_toint(l_5) );

    as_list_destroy(d);
    
    as_list_destroy(&l);
}

TEST( types_arraylist_iterator, "as_arraylistlist w/ iterator ops" ) {

    as_list l;
    as_arraylist_init(&l,10,10);
    
    for ( int i = 1; i < 6; i++) {
        as_list_append(&l, (as_val *) as_integer_new(i));
    }

    assert_int_eq( as_list_size(&l), 5 );

    as_iterator * i = NULL;
    as_integer * v = NULL;


    i  = as_list_iterator_new(&l);

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
}
/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_arraylist, "as_arraylist" ) {
    suite_add( types_arraylist_empty );
    suite_add( types_arraylist_cap10_blk0 );
    suite_add( types_arraylist_cap10_blk10 );
    suite_add( types_arraylist_list );
    suite_add( types_arraylist_iterator );
}
