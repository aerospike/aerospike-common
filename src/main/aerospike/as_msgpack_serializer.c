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
#include <aerospike/as_msgpack_serializer.h>
#include <aerospike/as_serializer.h>
#include <aerospike/as_types.h>

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void as_msgpack_serializer_destroy(as_serializer *);
static int  as_msgpack_serializer_serialize(as_serializer *, as_val *, as_buffer *);
static int  as_msgpack_serializer_deserialize(as_serializer *, as_buffer *, as_val **);

/******************************************************************************
 * VARIABLES
 *****************************************************************************/

static const as_serializer_hooks as_msgpack_serializer_hooks = {
    .destroy        = as_msgpack_serializer_destroy,
    .serialize      = as_msgpack_serializer_serialize,
    .deserialize    = as_msgpack_serializer_deserialize
};

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_serializer * as_msgpack_new() {
    return as_serializer_new(NULL, &as_msgpack_serializer_hooks);
}

as_serializer * as_msgpack_init(as_serializer * s)
 {
    as_serializer_init(s, NULL, &as_msgpack_serializer_hooks);
    return s;
}

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void as_msgpack_serializer_destroy(as_serializer * s) {
    return;
}

static int as_msgpack_serializer_serialize(as_serializer * s, as_val * v, as_buffer * buff) {
    msgpack_sbuffer sbuf;
    msgpack_packer  pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    as_msgpack_pack_val(&pk, v);

    buff->data = (uint8_t *) sbuf.data;
    buff->size = sbuf.size;
    buff->capacity = sbuf.alloc;

    return 0;
}

static int as_msgpack_serializer_deserialize(as_serializer * s, as_buffer * buff, as_val ** v) {
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);

    size_t offset = 0;

    if ( msgpack_unpack_next(&msg, (uint8_t *) buff->data, buff->size, &offset) ) {
        as_msgpack_object_to_val(&msg.data, v);
    }

    msgpack_unpacked_destroy(&msg);
    return 0;
}
