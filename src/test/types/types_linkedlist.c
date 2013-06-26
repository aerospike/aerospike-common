
#include "../test.h"

#include <aerospike/as_integer.h>
#include <aerospike/as_linkedlist.h>
#include <aerospike/as_linkedlist_iterator.h>
#include <aerospike/as_list.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_linkedlist_empty, "as_linkedlist is empty" ) {
    as_linkedlist l;
    as_linkedlist_init(&l,NULL,NULL);
    assert( l.head == NULL );
    assert( l.tail == NULL );
}

static void print_list(const char *msg, const as_list *l) {
	char *s = as_val_tostring(l);
	fprintf(stderr, "%s %s\n",msg,s);
	free(s);
}

TEST( types_linkedlist_1, "as_linkedlist ops" ) {

    int rc = 0;

    as_linkedlist l;
    as_linkedlist_init(&l,NULL,NULL);

    assert_null( l.head );
    assert_null( l.tail );

    assert_int_eq( as_linkedlist_size(&l), 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_linkedlist_append(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, 0 );
        assert_int_eq( as_linkedlist_size( &l), i );
    }
    
    for ( int i = 6; i < 11; i++) {
        rc = as_linkedlist_prepend(&l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, 0 );
        assert_int_eq( as_linkedlist_size(&l), i );
    }

    as_linkedlist * t = as_linkedlist_take(&l, 5);
    assert_int_eq( as_linkedlist_size(t), 5 );

    as_integer * t_head = (as_integer *) as_linkedlist_head(t);
    as_integer * l_head = (as_integer *) as_linkedlist_head(&l);

    assert( as_integer_toint(t_head) == as_integer_toint(l_head) );

    as_linkedlist_destroy(t);
    
    as_linkedlist * d = as_linkedlist_drop(&l, 5);
    assert_int_eq( as_linkedlist_size(d), 5 );

    as_integer * d_0 = (as_integer *) as_linkedlist_get(d, 0);
    as_integer * l_5 = (as_integer *) as_linkedlist_get(&l, 5);

    assert( d_0->value == l_5->value );
    
    as_linkedlist_destroy(d);

    as_linkedlist_destroy(&l);

}

TEST( types_linkedlist_2, "as_linkedlist w/ as_list ops" ) {

    int rc = 0;

    as_linkedlist l;
    as_linkedlist_init(&l,NULL,NULL);

    assert_null( l.head );
    assert_null( l.tail );

    assert_int_eq( as_list_size((as_list *) &l), 0 );

    for ( int i = 1; i < 6; i++) {
        rc = as_list_append((as_list *) &l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, 0 );
        assert_int_eq( as_list_size((as_list *) &l), i );
    }
    
    for ( int i = 6; i < 11; i++) {
        rc = as_list_prepend((as_list *) &l, (as_val *) as_integer_new(i));
        assert_int_eq( rc, 0 );
        assert_int_eq( as_list_size((as_list *) &l), i );
    }

    as_list * t = as_list_take((as_list *) &l, 5);
    assert_int_eq( as_list_size(t), 5 );

    as_integer * t_head = (as_integer *) as_list_head((as_list *) t);
    as_integer * l_head = (as_integer *) as_list_head((as_list *) &l);

    assert( as_integer_toint(t_head) == as_integer_toint(l_head) );

    as_list_destroy((as_list *) t);
    
    as_list * d = as_list_drop((as_list *) &l, 5);
    assert_int_eq( as_list_size((as_list *) d), 5 );

    as_integer * d_0 = (as_integer *) as_list_get((as_list *) d, 0);
    as_integer * l_5 = (as_integer *) as_list_get((as_list *) &l, 5);

    assert( d_0->value == l_5->value );
    
    as_list_destroy((as_list *) d);

    as_list_destroy((as_list *) &l);

}

TEST( types_linkedlist_3, "as_linkedlist w/ as_iterator ops" ) {
    
    as_linkedlist l;
    as_linkedlist_init(&l,NULL,NULL);
    
    for ( int i = 1; i < 6; i++) {
        as_list_append((as_list *) &l, (as_val *) as_integer_new(i));
    }

    assert_int_eq( as_list_size((as_list *) &l), 5 );


    as_iterator * i = (as_iterator *) as_linkedlist_iterator_new(&l);

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
    
    as_list_destroy((as_list *) &l);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_linkedlist, "as_linkedlist" ) {
    suite_add( types_linkedlist_empty );
    suite_add( types_linkedlist_1 );
    suite_add( types_linkedlist_2 );
    suite_add( types_linkedlist_3 );
}
