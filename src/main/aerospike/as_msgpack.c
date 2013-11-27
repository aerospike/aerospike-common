/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <msgpack.h>

#include <aerospike/as_msgpack.h>
#include <aerospike/as_serializer.h>
#include <aerospike/as_types.h>

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_msgpack_pack_boolean(msgpack_packer *, as_boolean *);
static int as_msgpack_pack_integer(msgpack_packer *, as_integer *);
static int as_msgpack_pack_string(msgpack_packer *, as_string *);
static int as_msgpack_pack_bytes(msgpack_packer *, as_bytes *);
static int as_msgpack_pack_list(msgpack_packer *, as_list *);
static int as_msgpack_pack_map(msgpack_packer *, as_map *);
static int as_msgpack_pack_rec(msgpack_packer *, as_rec *);
static int as_msgpack_pack_pair(msgpack_packer *, as_pair *);

static int as_msgpack_boolean_to_val(bool, as_val **);
static int as_msgpack_integer_to_val(int64_t, as_val **);
static int as_msgpack_raw_to_val(msgpack_object_raw *, as_val **);
static int as_msgpack_array_to_val(msgpack_object_array *, as_val **);
static int as_msgpack_map_to_val(msgpack_object_map *, as_val **);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_msgpack_pack_val(msgpack_packer * pk, as_val * v) {
    if ( v == NULL ) return 1;
    switch( as_val_type(v) ) {
        case AS_BOOLEAN : return as_msgpack_pack_boolean(pk, (as_boolean *) v);
        case AS_INTEGER : return as_msgpack_pack_integer(pk, (as_integer *) v);
        case AS_STRING  : return as_msgpack_pack_string(pk, (as_string *) v);
        case AS_BYTES   : return as_msgpack_pack_bytes(pk, (as_bytes *) v);
        case AS_LIST    : return as_msgpack_pack_list(pk, (as_list *) v);
        case AS_MAP     : return as_msgpack_pack_map(pk, (as_map *) v);
        case AS_REC     : return as_msgpack_pack_rec(pk, (as_rec *) v);
        case AS_PAIR    : return as_msgpack_pack_pair(pk, (as_pair *) v);
        default         : return 2;
    }
}

int as_msgpack_object_to_val(msgpack_object * object, as_val ** val) {
    if ( object == NULL ) return 1;
    switch( object->type ) {
        case MSGPACK_OBJECT_BOOLEAN             : return as_msgpack_boolean_to_val(object->via.boolean, val);
        case MSGPACK_OBJECT_POSITIVE_INTEGER    : return as_msgpack_integer_to_val((int64_t) object->via.u64, val);
        case MSGPACK_OBJECT_NEGATIVE_INTEGER    : return as_msgpack_integer_to_val((int64_t) object->via.i64, val);
        case MSGPACK_OBJECT_RAW                 : return as_msgpack_raw_to_val(&object->via.raw, val);
        case MSGPACK_OBJECT_ARRAY               : return as_msgpack_array_to_val(&object->via.array, val);
        case MSGPACK_OBJECT_MAP                 : return as_msgpack_map_to_val(&object->via.map, val);
        // case MSGPACK_OBJECT_POSITIVE_INTEGER    : {
            // The assumption is that uint is used as an identifier for a specific type of object.
            // So we will read the uint and baed on the value, deserialize to that object.        
            // case AS_REC     : return as_msgpack_unpack_rec      (buff, msg, offset, (as_rec **) v);
            // case AS_PAIR    : return as_msgpack_unpack_pair     (buff, msg, offset, (as_pair **) v);
        // }
        default                                 : return 2;
    }
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_msgpack_pack_boolean(msgpack_packer * pk, as_boolean * b) {
    return as_boolean_tobool(b) ? msgpack_pack_true(pk) : msgpack_pack_false(pk);
}

static int as_msgpack_pack_integer(msgpack_packer * pk, as_integer * i) {
    return msgpack_pack_int64(pk, as_integer_toint(i));
}

static int as_msgpack_pack_string(msgpack_packer * pk, as_string * s) {
    int len = as_string_len(s) + 1;
    uint8_t tbuf[len];
    tbuf[0] = AS_BYTES_TYPE_STRING;
    memcpy(tbuf+1, as_string_tostring(s),len-1);

    int rc = msgpack_pack_raw(pk, len);
    if ( rc ) return rc;
    return msgpack_pack_raw_body(pk, tbuf, len);
}

static int as_msgpack_pack_bytes(msgpack_packer * pk, as_bytes * b) {
    int len = as_bytes_len(b) + 1;
    uint8_t tbuf[len];
    tbuf[0] = as_bytes_get_type(b);
    memcpy(tbuf+1, as_bytes_tobytes(b),len-1);

    int rc = msgpack_pack_raw(pk, len);
    if ( rc ) return rc;
    return msgpack_pack_raw_body(pk, tbuf, len);
}

static int as_msgpack_pack_list(msgpack_packer * pk, as_list * l) {
    
    int rc = msgpack_pack_array(pk, as_list_size(l));
    if ( rc ) return rc;

    as_iterator i;
    as_list_iterator_init(&i, l);
    while ( as_iterator_has_next(&i) ) {
        as_val * val = (as_val *) as_iterator_next(&i);

        int rc = as_msgpack_pack_val(pk, val);
        if ( rc ) {
            rc = 2;
            break;
        }
    }
    as_iterator_destroy(&i);

    return rc;
}

static int as_msgpack_pack_map(msgpack_packer * pk, as_map * m) {
    int rc = msgpack_pack_map(pk, as_map_size(m));
    if ( rc ) return rc;

    as_iterator i;
    as_map_iterator_init(&i, m);
    while ( as_iterator_has_next(&i) ) {
        as_pair * p = (as_pair *) as_iterator_next(&i);

        if ( !p ) {
            rc = 2;
            break;
        }
        rc = as_msgpack_pack_val(pk, as_pair_1(p));
        if ( rc ) break;
        rc = as_msgpack_pack_val(pk, as_pair_2(p));
        if ( rc ) break;
    }

    as_iterator_destroy(&i);

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

static int as_msgpack_boolean_to_val(bool b, as_val ** v) {
    *v = (as_val *) as_boolean_new(b);
    return 0;
}

static int as_msgpack_integer_to_val(int64_t i, as_val ** v) {
    *v = (as_val *) as_integer_new(i);
    return 0;
}

static int as_msgpack_raw_to_val(msgpack_object_raw * r, as_val ** v) {
    const char * raw = r->ptr;
    *v = 0;
    // strings are special
    if (*raw == AS_BYTES_TYPE_STRING) {
        *v = (as_val *) as_string_new(strndup(raw+1,r->size - 1),true);
    }
    // everything else encoded as a bytes with the type set
    else {
        int len = r->size - 1;
        uint8_t *buf = malloc(len);
        if (!buf) {
            *v = NULL; 
            return 0;
        }
        memcpy(buf, raw+1, len);
        as_bytes *b = as_bytes_new(buf, len, true /*ismalloc*/);
        if (b)
            as_bytes_set_type( b, (as_bytes_type) *raw );
        *v = (as_val *) b;
    }
    return 0;
}

static int as_msgpack_array_to_val(msgpack_object_array * a, as_val ** v) {
    as_list * l = as_arraylist_new(a->size,8);
    for ( int i = 0; i < a->size; i++) {
        msgpack_object * o = a->ptr + i;
        as_val * val = NULL;
        as_msgpack_object_to_val(o, &val);
        if ( val != NULL ) {
            as_list_set(l, i, val);
        }
    }
    *v = (as_val *) l;
    return 0;
}

static int as_msgpack_map_to_val(msgpack_object_map * o, as_val ** v) {
	uint32_t buckets = o->size < 32 ? 32 : (o->size/8) * 8 + 8;
    as_map * m = as_hashmap_new(32);
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
