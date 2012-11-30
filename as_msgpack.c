#include "as_msgpack.h"

int as_msgpack_pack_boolean(msgpack_packer * pk, as_boolean * b) {
    return as_boolean_tobool(b) ? msgpack_pack_true(pk) : msgpack_pack_false(pk);
}

int as_msgpack_pack_integer(msgpack_packer * pk, as_integer * i) {
    return msgpack_pack_int64(pk, as_integer_toint(i));
}

int as_msgpack_pack_string(msgpack_packer * pk, as_string * s) {
    int rc = msgpack_pack_raw(pk, as_string_len(s));
    if ( rc ) return rc;
    return msgpack_pack_raw_body(pk, as_string_tostring(s), as_string_len(s));
}

int as_msgpack_pack_list(msgpack_packer * pk, as_list * l) {
    int rc = msgpack_pack_array(pk, as_list_size(l));
    if ( rc ) return rc;

    as_iterator * i = as_list_iterator(l);
    while ( as_iterator_has_next(i) ) {
        as_val * val = (as_val *) as_iterator_next(i);
        if ( as_msgpack_pack_val(pk, val) ) {
            as_iterator_free(i);
            return 2;
        }
    }
    as_iterator_free(i);

    return 0;
}

int as_msgpack_pack_map(msgpack_packer * pk, as_map * m) {
    int rc = msgpack_pack_map(pk, as_map_size(m));
    if ( rc ) return rc;

    as_iterator * i = as_map_iterator(m);
    while ( as_iterator_has_next(i) ) {
        as_pair * p = (as_pair *) as_iterator_next(i);
        if ( !p ) {
            rc = 2;
            goto Cleanup;
        }
        rc = as_msgpack_pack_val(pk, as_pair_1(p));
        if ( rc ) goto Cleanup;
        rc = as_msgpack_pack_val(pk, as_pair_2(p));
        if ( rc ) goto Cleanup;
    }

Cleanup:
    as_iterator_free(i);

    return rc;
}

int as_msgpack_pack_rec(msgpack_packer * pk, as_rec * r) {
    return 1;
}

int as_msgpack_pack_pair(msgpack_packer * pk, as_pair * p) {
    int rc = msgpack_pack_array(pk, 2);
    if ( rc ) return rc;
    rc = as_msgpack_pack_val(pk, as_pair_1(p));
    if ( rc ) return rc;
    rc = as_msgpack_pack_val(pk, as_pair_2(p));
    return rc;
}

int as_msgpack_pack_val(msgpack_packer * pk, as_val * v) {
    if ( v == NULL ) return 1;
    switch( as_val_type(v) ) {
        case AS_BOOLEAN : return as_msgpack_pack_boolean(pk, (as_boolean *) v);
        case AS_INTEGER : return as_msgpack_pack_integer(pk, (as_integer *) v);
        case AS_STRING  : return as_msgpack_pack_string(pk, (as_string *) v);
        case AS_LIST    : return as_msgpack_pack_list(pk, (as_list *) v);
        case AS_MAP     : return as_msgpack_pack_map(pk, (as_map *) v);
        case AS_REC     : return as_msgpack_pack_rec(pk, (as_rec *) v);
        case AS_PAIR    : return as_msgpack_pack_pair(pk, (as_pair *) v);
        default         : return 2;
    }
}

