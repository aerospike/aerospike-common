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

#include "internal.h"

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_msgpack_pack_nil(msgpack_packer *);
static int as_msgpack_pack_boolean(msgpack_packer *, as_boolean *);
static int as_msgpack_pack_integer(msgpack_packer *, as_integer *);
static int as_msgpack_pack_string(msgpack_packer *, as_string *);
static int as_msgpack_pack_bytes(msgpack_packer *, as_bytes *);
static int as_msgpack_pack_list(msgpack_packer *, as_list *);
static int as_msgpack_pack_map(msgpack_packer *, as_map *);
static int as_msgpack_pack_rec(msgpack_packer *, as_rec *);
static int as_msgpack_pack_pair(msgpack_packer *, as_pair *);

static int as_msgpack_nil_to_val(as_val ** v);
static int as_msgpack_boolean_to_val(bool, as_val **);
static int as_msgpack_integer_to_val(int64_t, as_val **);
static int as_msgpack_raw_to_val(msgpack_object_raw *, as_val **);
static int as_msgpack_array_to_val(msgpack_object_array *, as_val **);
static int as_msgpack_map_to_val(msgpack_object_map *, as_val **);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_msgpack_pack_val(msgpack_packer * pk, as_val * val)
{
#if LOG_ENABLED==1
	char * v = as_val_tostring(val);
	LOG("as_msgpack_pack_val : start : %s",v);
	free(v);
#endif

	int rc = 0;

	if ( val == NULL ) {
		LOG("as_msgpack_pack_val : value is null");
		rc = 1;
	}
	else {	
		switch( as_val_type(val) ) {
			case AS_NIL : 
				rc = as_msgpack_pack_nil(pk);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_nil : %d", rc);
				break;
			case AS_BOOLEAN : 
				rc = as_msgpack_pack_boolean(pk, (as_boolean *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_boolean : %d", rc);
				break;
			case AS_INTEGER : 
				rc = as_msgpack_pack_integer(pk, (as_integer *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_integer : %d", rc);
				break;
			case AS_STRING : 
				rc = as_msgpack_pack_string(pk, (as_string *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_string : %d", rc);
				break;
			case AS_BYTES : 
				rc = as_msgpack_pack_bytes(pk, (as_bytes *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_bytes : %d", rc);
				break;
			case AS_LIST : 
				rc = as_msgpack_pack_list(pk, (as_list *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_list : %d", rc);
				break;
			case AS_MAP : 
				rc = as_msgpack_pack_map(pk, (as_map *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_map : %d", rc);
				break;
			case AS_REC : 
				rc = as_msgpack_pack_rec(pk, (as_rec *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_rec : %d", rc);
				break;
			case AS_PAIR : 
				rc = as_msgpack_pack_pair(pk, (as_pair *) val);
				LOG_COND(rc, "as_msgpack_pack_val : as_msgpack_pack_pair : %d", rc);
				break;
			default : 
				rc = 2;
		}
	}

	LOG("as_msgpack_pack_val : end : %d", rc);
	return rc;
}

int as_msgpack_object_to_val(msgpack_object * object, as_val ** val) 
{
	if ( object == NULL ) return 1;
	switch( object->type ) {
		case MSGPACK_OBJECT_NIL             	: return as_msgpack_nil_to_val(val);
		case MSGPACK_OBJECT_BOOLEAN             : return as_msgpack_boolean_to_val(object->via.boolean, val);
		case MSGPACK_OBJECT_POSITIVE_INTEGER    : return as_msgpack_integer_to_val((int64_t) object->via.u64, val);
		case MSGPACK_OBJECT_NEGATIVE_INTEGER    : return as_msgpack_integer_to_val((int64_t) object->via.i64, val);
		case MSGPACK_OBJECT_RAW                 : return as_msgpack_raw_to_val(&object->via.raw, val);
		case MSGPACK_OBJECT_ARRAY               : return as_msgpack_array_to_val(&object->via.array, val);
		case MSGPACK_OBJECT_MAP                 : return as_msgpack_map_to_val(&object->via.map, val);
		default                                 : return 2;
	}
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_msgpack_pack_nil(msgpack_packer * pk)
{
	LOG("as_msgpack_pack_nil : start");
	int rc = msgpack_pack_nil(pk);
	LOG_COND(rc, "as_msgpack_pack_nil : msgpack_pack_nil : %d",rc);
	LOG("as_msgpack_pack_nil : end : %d", rc);
	return rc;
}

static int as_msgpack_pack_boolean(msgpack_packer * pk, as_boolean * b)
{
	LOG("as_msgpack_pack_boolean : start");
	int rc = 0;

	if ( as_boolean_get(b) == true ) {
		rc = msgpack_pack_true(pk);
		LOG_COND(rc, "as_msgpack_pack_boolean : msgpack_pack_true : %d",rc);
	}
	else {
		rc = msgpack_pack_false(pk);
		LOG_COND(rc, "as_msgpack_pack_boolean : msgpack_pack_false : %d",rc);
	}

	LOG("as_msgpack_pack_boolean : end : %d", rc);
	return rc;
}

static int as_msgpack_pack_integer(msgpack_packer * pk, as_integer * i)
{
	LOG("as_msgpack_pack_integer : start");
	int rc = 0;

	rc = msgpack_pack_int64(pk, as_integer_get(i));
	LOG_COND(rc, "as_msgpack_pack_integer : msgpack_pack_int64 : %d",rc);

	LOG("as_msgpack_pack_integer : end : %d", rc);
	return rc;
}

static int as_msgpack_pack_string(msgpack_packer * pk, as_string * s)
{
	LOG("as_msgpack_pack_string : start : %p", s);
	int rc = 0;
	
	uint32_t len = as_string_len(s) + 1;
	uint8_t * raw = alloca(len);

	raw[0] = AS_BYTES_STRING;
	memcpy(raw + 1, s->value, s->len);

	rc = msgpack_pack_raw(pk, len);
	LOG_COND(rc, "as_msgpack_pack_string : msgpack_pack_raw : %d",rc);
	if ( rc == 0 ) {
		rc = msgpack_pack_raw_body(pk, raw, len);
		LOG_COND(rc, "as_msgpack_pack_string : msgpack_pack_raw_body : %d",rc);
	}

	LOG("as_msgpack_pack_string : end : %d", rc);
	return rc;
}

static int as_msgpack_pack_bytes(msgpack_packer * pk, as_bytes * b)
{
	LOG("as_msgpack_pack_bytes : start");
	int rc = 0;

	uint32_t  len = b->size + 1;
	uint8_t * raw = alloca(len);
	raw[0] = b->type;
	memcpy(raw + 1, b->value, b->size);

	rc = msgpack_pack_raw(pk, len);
	LOG_COND(rc, "as_msgpack_pack_bytes : msgpack_pack_raw : %d",rc);
	if ( rc == 0 ) {	
		rc = msgpack_pack_raw_body(pk, raw, len);
		LOG_COND(rc, "as_msgpack_pack_bytes : msgpack_pack_raw_body : %d",rc);
	}

	LOG("as_msgpack_pack_bytes : end : %d", rc);
	return rc;
}

static bool as_msgpack_pack_list_foreach(as_val * val, void * udata)
{
	LOG("as_msgpack_pack_list_foreach : start");
	int rc = 0;

	msgpack_packer * pk = (msgpack_packer *) udata;

	rc = as_msgpack_pack_val(pk, val);
	LOG_COND(rc, "as_msgpack_pack_list_foreach : as_msgpack_pack_val : %d",rc);

	LOG("as_msgpack_pack_list_foreach : %d", rc);
	return rc == 0;
}

static int as_msgpack_pack_list(msgpack_packer * pk, as_list * l)
{
	LOG("as_msgpack_pack_list : start");
	int rc = 0;

	rc = msgpack_pack_array(pk, as_list_size(l));
	LOG_COND(rc, "as_msgpack_pack_list : msgpack_pack_array : %d",rc);

	if ( rc == 0 ) {
		rc = as_list_foreach(l, as_msgpack_pack_list_foreach, pk) == true ? 0 : 1;
		LOG_COND(rc, "as_msgpack_pack_map : as_map_foreach : %d", rc);
	}

	LOG("as_msgpack_pack_list : end : %d",rc);
	return rc;
}

static bool as_msgpack_pack_map_foreach(const as_val * key, const as_val * val, void * udata)
{
#if LOG_ENABLED==1
	char * k = as_val_tostring(key);
	char * v = as_val_tostring(val);
	LOG("as_msgpack_pack_map_foreach : start : %s=%s",k,v);
	free(k);
	free(v);
#endif

	int rc = 0;

	msgpack_packer * pk = (msgpack_packer *) udata;

	rc = as_msgpack_pack_val(pk, (as_val *) key);
	LOG_COND(rc, "as_msgpack_pack_map_foreach : as_msgpack_pack_val : %d",rc);

	if ( rc == 0 ) {	
		rc = as_msgpack_pack_val(pk, (as_val *) val);
		LOG_COND(rc, "as_msgpack_pack_map_foreach : as_msgpack_pack_val : %d",rc);
	}

	LOG("as_msgpack_pack_map_foreach : end : %d", rc);
	return true;//rc == 0;
}

static int as_msgpack_pack_map(msgpack_packer * pk, as_map * m)
{
	LOG("as_msgpack_pack_map : start : size=%d", as_map_size(m));
	int rc = 0;

	rc = msgpack_pack_map(pk, as_map_size(m));
	LOG_COND(rc, "as_msgpack_pack_map : msgpack_pack_map : %d",rc);
	if ( rc == 0 ) {
		rc = as_map_foreach(m, as_msgpack_pack_map_foreach, pk) == true ? 0 : 1;
		LOG_COND(rc, "as_msgpack_pack_map : as_map_foreach : %d", rc);
	}

	LOG("as_msgpack_pack_map : end : %d", rc);
	return rc;
}

static int as_msgpack_pack_rec(msgpack_packer * pk, as_rec * r)
{
	LOG("as_msgpack_pack_rec : NOP");
	return 1;
}

static int as_msgpack_pack_pair(msgpack_packer * pk, as_pair * p)
{
	LOG("as_msgpack_pack_pair : start");
	int rc = 0;

	rc = msgpack_pack_array(pk, 2);
	LOG_COND(rc, "as_msgpack_pack_pair : msgpack_pack_array : %d",rc);

	if ( rc == 0 ) {
		rc = as_msgpack_pack_val(pk, as_pair_1(p));
		LOG_COND(rc, "as_msgpack_pack_pair : as_msgpack_pack_val : %d",rc);

		if ( rc == 0 ) {
			rc = as_msgpack_pack_val(pk, as_pair_2(p));
			LOG_COND(rc, "as_msgpack_pack_pair : as_msgpack_pack_val : %d",rc);
		}
	}

	LOG("as_msgpack_pack_pair : end : %d", rc);
	return rc;
}


static int as_msgpack_nil_to_val(as_val ** v)
{
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
	*v = (as_val *) &as_nil;
	return 0;
}

static int as_msgpack_boolean_to_val(bool b, as_val ** v)
{
	// Aerospike does not support Boolean, so we convert it to Integer
	*v = (as_val *) as_integer_new(b == true ? 1 : 0);
	return 0;
}

static int as_msgpack_integer_to_val(int64_t i, as_val ** v)
{
	*v = (as_val *) as_integer_new(i);
	return 0;
}

static int as_msgpack_raw_to_val(msgpack_object_raw * r, as_val ** v)
{
	const char * raw = r->ptr;
	*v = 0;
	// strings are special
	if (*raw == AS_BYTES_STRING) {
		*v = (as_val *) as_string_new(strndup(raw+1,r->size - 1),true);
	}
	// everything else encoded as a bytes with the type set
	else {
		int len = r->size - 1;
		uint8_t *buf = malloc(len);
		memcpy(buf, raw+1, len);
		as_bytes *b = as_bytes_new_wrap(buf, len, true);
		if ( b ) {
			b->type = (as_bytes_type) *raw;
		}
		*v = (as_val *) b;
	}
	return 0;
}

static int as_msgpack_array_to_val(msgpack_object_array * a, as_val ** v)
{
	as_arraylist * l = as_arraylist_new(a->size, 8);
	for ( int i = 0; i < a->size; i++) {
		msgpack_object * o = a->ptr + i;
		as_val * val = NULL;
		as_msgpack_object_to_val(o, &val);
		if ( val != NULL ) {
			as_arraylist_set(l, i, val);
		}
	}
	*v = (as_val *) l;
	return 0;
}

static int as_msgpack_map_to_val(msgpack_object_map * o, as_val ** v)
{
	as_hashmap * m = as_hashmap_new(32);
	for ( int i = 0; i < o->size; i++) {
		msgpack_object_kv * kv = o->ptr + i;
		as_val * key = NULL;
		as_val * val = NULL;
		as_msgpack_object_to_val(&kv->key, &key);
		as_msgpack_object_to_val(&kv->val, &val);
		if ( key != NULL && val != NULL ) {
			as_hashmap_set(m, key, val);
		}
	}
	*v = (as_val *) m;
	return 0;
}
