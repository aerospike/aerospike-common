/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <aerospike/as_msgpack.h>
#include <aerospike/as_msgpack_serializer.h>
#include <aerospike/as_serializer.h>
#include <aerospike/as_types.h>

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void     as_msgpack_serializer_destroy(as_serializer *);
static int      as_msgpack_serializer_serialize(as_serializer *, as_val *, as_buffer *);
static int32_t	as_msgpack_serializer_serialize_presized(as_serializer *, const as_val *, uint8_t *);
static int      as_msgpack_serializer_deserialize(as_serializer *, as_buffer *, as_val **);
static uint32_t as_msgpack_serializer_serialize_getsize(as_serializer *, as_val *);

/******************************************************************************
 * VARIABLES
 *****************************************************************************/

static const as_serializer_hooks as_msgpack_serializer_hooks = {
    .destroy           = as_msgpack_serializer_destroy,
    .serialize         = as_msgpack_serializer_serialize,
	.serialize_presized= as_msgpack_serializer_serialize_presized,
    .deserialize       = as_msgpack_serializer_deserialize,
    .serialize_getsize = as_msgpack_serializer_serialize_getsize,
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

static uint32_t as_msgpack_serializer_serialize_getsize(as_serializer * s, as_val * v) {
	as_packer packer;
	// No buffer means the request is for size
	packer.buffer   = NULL;
	packer.capacity = 0;
	packer.offset   = 0;
	packer.head     = 0;
	packer.tail     = 0;
	int rc = as_pack_val(&packer, v);
	if (rc)
		return 0;
	return packer.offset;
}

static int32_t as_msgpack_serializer_serialize_presized(as_serializer *s, const as_val *v, uint8_t *buf)
{
	as_packer packer = {
		.buffer = buf,
		// Prevent extra allocation.
		// buf should contain (pre-sized) space for the unpacking.
		.capacity = INT_MAX,
		.offset = 0,
		.head = 0,
		.tail = 0,
	};

	if (as_pack_val(&packer, v) != 0) {
		return -1;
	}

	return packer.offset;
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
