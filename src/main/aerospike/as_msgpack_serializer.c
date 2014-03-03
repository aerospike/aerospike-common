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

as_serializer * as_msgpack_init(as_serializer * s) {
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
	as_packer packer;
	packer.buffer = (unsigned char *) cf_malloc(AS_PACKER_BUFFER_SIZE);
	packer.capacity = AS_PACKER_BUFFER_SIZE;
	packer.offset = 0;
	packer.head = 0;
	packer.tail = 0;
	
	if (! packer.buffer) {
		return 1;
	}
	
    int rc = as_pack_val(&packer, v);
	
	if (rc) {
		// Cleanup buffers on error.
		as_packer_buffer * p = packer.head;
		as_packer_buffer * tmp;
		
		while (p) {
			tmp = p;
			p = p->next;
			
			// Free buffer entry.
			cf_free(tmp->buffer);
			cf_free(tmp);
		}
		
		// Free main buffer.
		cf_free(packer.buffer);
		return rc;
	}

	if (packer.head) {
		// Combine buffers into a single contiguous buffer.
		as_packer_buffer * p = packer.head;
		int size = packer.offset;
		
		while (p) {
			size += p->length;
			p = p->next;
		}
		
		unsigned char * target = (unsigned char *) cf_malloc(size);
		p = packer.head;
		int offset = 0;
		
		while (p) {
			memcpy(target + offset, p->buffer, p->length);
			offset += p->length;
			
			as_packer_buffer * tmp = p;
			p = p->next;
			
			// Free original buffer entry.
			cf_free(tmp->buffer);
			cf_free(tmp);
		}
		
		memcpy(target + offset, packer.buffer, packer.offset);
		
		// Free original main buffer.
		cf_free(packer.buffer);
		
		// Transfer new buffer.
		buff->data = target;
		buff->size = size;
		buff->capacity = size;
	}
	else {
		// Use existing buffer directly.
		buff->data = packer.buffer;
		buff->size = packer.offset;
		buff->capacity = packer.capacity;
	}
	return 0;
}

static int as_msgpack_serializer_deserialize(as_serializer * s, as_buffer * buff, as_val ** v) {
	
	as_unpacker unpacker;
	unpacker.buffer = buff->data;
	unpacker.length = buff->size;
	unpacker.offset = 0;
	
	return as_unpack_val(&unpacker, v);
}
