#include "as_msgpack.h"
#include "as_serializer.h"
#include "as_types.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_msgpack_pack_boolean(msgpack_packer *, as_boolean *);
static int as_msgpack_pack_integer(msgpack_packer *, as_integer *);
static int as_msgpack_pack_string(msgpack_packer *, as_string *);
static int as_msgpack_pack_list(msgpack_packer *, as_list *);
static int as_msgpack_pack_map(msgpack_packer *, as_map *);
static int as_msgpack_pack_rec(msgpack_packer *, as_rec *);
static int as_msgpack_pack_pair(msgpack_packer *, as_pair *);
static int as_msgpack_pack_val(msgpack_packer *, as_val *);

static int as_msgpack_boolean_to_val(bool, as_val **);
static int as_msgpack_integer_to_val(int64_t, as_val **);
static int as_msgpack_string_to_val(msgpack_object_raw *, as_val **);
static int as_msgpack_array_to_val(msgpack_object_array *, as_val **);
static int as_msgpack_map_to_val(msgpack_object_map *, as_val **);
static int as_msgpack_object_to_val(msgpack_object *, as_val **);

static int as_msgpack_free(as_serializer *);
static int as_msgpack_serialize(as_serializer *, as_val *, as_buffer *);
static int as_msgpack_deserialize(as_serializer *, as_buffer *, as_val **);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_serializer_hooks as_msgpack_serializer_hooks = {
    .free           = as_msgpack_free,
    .serialize      = as_msgpack_serialize,
    .deserialize    = as_msgpack_deserialize
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

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


static int as_msgpack_boolean_to_val(bool b, as_val ** v) {
    *v = (as_val *) as_boolean_new(b);
    return 0;
}

static int as_msgpack_integer_to_val(int64_t i, as_val ** v) {
    *v = (as_val *) as_integer_new(i);
    return 0;
}

static int as_msgpack_string_to_val(msgpack_object_raw * r, as_val ** v) {
    *v = (as_val *) as_string_new(strndup(r->ptr, sizeof(char) * r->size));
    return 0;
}

static int as_msgpack_array_to_val(msgpack_object_array * a, as_val ** v) {
    as_list * l = as_arraylist_new(a->size,0);
    for ( int i = 0; i < a->size; i++) {
        msgpack_object * o = a->ptr + i;
        as_val * val = NULL;
        as_msgpack_object_to_val(o, &val);
        if ( val != NULL ) {
            as_list_append(l, val);
        }
    }
    *v = (as_val *) l;
    return 0;
}

static int as_msgpack_map_to_val(msgpack_object_map * o, as_val ** v) {
    as_map * m = as_hashmap_new(o->size);
    for ( int i = 0; i < o->size; i++) {
        msgpack_object_kv * kv = o->ptr + i;
        as_val * key = NULL;
        as_val * val = NULL;
        as_msgpack_object_to_val(&kv->key, &key);
        as_msgpack_object_to_val(&kv->val, &val);
        if ( key != NULL && val != NULL ) {
            as_map_set(m, key, val);
        }
    }
    *v = (as_val *) m;
    return 0;
}

static int as_msgpack_object_to_val(msgpack_object * object, as_val ** val) {
    if ( object == NULL ) return 1;
    switch( object->type ) {
        case MSGPACK_OBJECT_BOOLEAN             : return as_msgpack_boolean_to_val  (object->via.boolean, val);
        case MSGPACK_OBJECT_POSITIVE_INTEGER    : return as_msgpack_integer_to_val  ((int64_t) object->via.u64, val);
        case MSGPACK_OBJECT_NEGATIVE_INTEGER    : return as_msgpack_integer_to_val  ((int64_t) object->via.i64, val);
        case MSGPACK_OBJECT_RAW                 : return as_msgpack_string_to_val   (&object->via.raw, val);
        case MSGPACK_OBJECT_ARRAY               : return as_msgpack_array_to_val    (&object->via.array, val);
        case MSGPACK_OBJECT_MAP                 : return as_msgpack_map_to_val      (&object->via.map, val);
        // case MSGPACK_OBJECT_POSITIVE_INTEGER    : {
            // The assumption is that uint is used as an identifier for a specific type of object.
            // So we will read the uint and baed on the value, deserialize to that object.        
            // case AS_REC     : return as_msgpack_unpack_rec      (buff, msg, offset, (as_rec **) v);
            // case AS_PAIR    : return as_msgpack_unpack_pair     (buff, msg, offset, (as_pair **) v);
        // }
        default                                 : return 2;
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

    return 0;
}

static int as_msgpack_deserialize(as_serializer * s, as_buffer * buff, as_val ** v) {
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);

    size_t offset = 0;

    if ( msgpack_unpack_next(&msg, buff->data, buff->size, &offset) ) {
        as_msgpack_object_to_val(&msg.data, v);
    }

    msgpack_unpacked_destroy(&msg);
    return 0;
}
