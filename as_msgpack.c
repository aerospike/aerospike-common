#include "as_msgpack.h"
#include "as_serializer.h"
#include "as_types.h"



static const as_serializer_hooks as_msgpack_serializer_hooks;

static int as_msgpack_pack_boolean(msgpack_packer *, as_boolean *);
static int as_msgpack_pack_integer(msgpack_packer *, as_integer *);
static int as_msgpack_pack_string(msgpack_packer *, as_string *);
static int as_msgpack_pack_list(msgpack_packer *, as_list *);
static int as_msgpack_pack_map(msgpack_packer *, as_map *);
static int as_msgpack_pack_rec(msgpack_packer *, as_rec *);
static int as_msgpack_pack_pair(msgpack_packer *, as_pair *);
static int as_msgpack_pack_val(msgpack_packer *, as_val *);


as_serializer * as_msgpack_new() {
    return as_serializer_new(NULL, &as_msgpack_serializer_hooks);
}

int as_msgpack_init(as_serializer * s) {
    as_serializer_init(s, NULL, &as_msgpack_serializer_hooks);
    return 0;
}

static int as_msgpack_pack_boolean(msgpack_packer * pk, as_boolean * b) {
    return as_boolean_tobool(b) ? msgpack_pack_true(pk) : msgpack_pack_false(pk);
}

static int as_msgpack_pack_integer(msgpack_packer * pk, as_integer * i) {
    return msgpack_pack_int64(pk, as_integer_toint(i));
}

static int as_msgpack_pack_string(msgpack_packer * pk, as_string * s) {
    int rc = msgpack_pack_raw(pk, as_string_len(s));
    if ( rc ) return rc;
    return msgpack_pack_raw_body(pk, as_string_tostring(s), as_string_len(s));
}

static int as_msgpack_pack_list(msgpack_packer * pk, as_list * l) {
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

static int as_msgpack_pack_map(msgpack_packer * pk, as_map * m) {
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

static int as_msgpack_pack_rec(msgpack_packer * pk, as_rec * r) {
    return 1;
}

static int as_msgpack_pack_pair(msgpack_packer * pk, as_pair * p) {
    int rc = msgpack_pack_array(pk, 2);
    if ( rc ) return rc;
    rc = as_msgpack_pack_val(pk, as_pair_1(p));
    if ( rc ) return rc;
    rc = as_msgpack_pack_val(pk, as_pair_2(p));
    return rc;
}

static int as_msgpack_pack_val(msgpack_packer * pk, as_val * v) {
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

static int as_msgpack_free(as_serializer * s) {
    return 0;
}

static int as_msgpack_serialize(as_serializer * s, as_val * v, as_buffer * buff) {
    msgpack_sbuffer sbuf;
    msgpack_packer  pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    as_msgpack_pack_val(&pk, v);

    buff->data = sbuf.data;
    buff->size = sbuf.size;
    buff->capacity = sbuf.alloc;

    // sbuf.data = NULL;
    // msgpack_sbuffer_destroy(&sbuf);
    return 0;
}


static int as_msgpack_deserialize(as_serializer * s, as_buffer * buff, as_val ** v) {
    return 0;
}

static const as_serializer_hooks as_msgpack_serializer_hooks = {
    .free           = as_msgpack_free,
    .serialize      = as_msgpack_serialize,
    .deserialize    = as_msgpack_deserialize
};

