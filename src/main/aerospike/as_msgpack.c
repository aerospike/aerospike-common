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
#include <aerospike/as_serializer.h>
#include <aerospike/as_types.h>
#include <citrusleaf/cf_byte_order.h>

#include "internal.h"

/******************************************************************************
 * PACK FUNCTIONS
 ******************************************************************************/

static int as_pack_resize(as_packer * pk, int length)
{
	// Add current buffer to linked list and allocate a new buffer
	as_packer_buffer* entry = (as_packer_buffer*) cf_malloc(sizeof(as_packer_buffer));
	
	if (entry == 0) {
		return -1;
	}
	entry->buffer = pk->buffer;
	entry->length = pk->offset;
	entry->next = 0;
	
	int size = (length > pk->capacity)? length : pk->capacity;
	pk->buffer = (unsigned char*) cf_malloc(size);
	
	if (pk->buffer == 0) {
		return -1;
	}
	pk->capacity = size;
	pk->offset = 0;
	
	if (pk->tail) {
		pk->tail->next = entry;
		pk->tail = entry;
	}
	else {
		pk->head = entry;
		pk->tail = entry;
	}
	return 0;
}

static inline int as_pack_append(as_packer * pk, const unsigned char * src, int length)
{
	if (pk->offset + length > pk->capacity) {
		if (as_pack_resize(pk, length)) {
			return -1;
		}
	}
	memcpy(pk->buffer + pk->offset, src, length);
	pk->offset += length;
	return 0;
}

static inline int as_pack_byte(as_packer * pk, uint8_t val) {
	if (pk->offset + 1 > pk->capacity) {
		if (as_pack_resize(pk, 1)) {
			return -1;
		}
	}
	*(pk->buffer + pk->offset) = val;
	pk->offset++;
	return 0;
}

static inline int as_pack_int8(as_packer * pk, unsigned char type, uint8_t val) {
	if (pk->offset + 2 > pk->capacity) {
		if (as_pack_resize(pk, 2)) {
			return -1;
		}
	}
	unsigned char* p = pk->buffer + pk->offset;
	*p++ = type;
	*p = val;
	pk->offset += 2;
	return 0;
}

static inline int as_pack_int16(as_packer * pk, unsigned char type, uint16_t val) {
	if (pk->offset + 3 > pk->capacity) {
		if (as_pack_resize(pk, 3)) {
			return -1;
		}
	}
	uint16_t swapped = cf_swap_to_be16(val);
	unsigned char* s = (unsigned char*)&swapped;
	unsigned char* p = pk->buffer + pk->offset;
	*p++ = type;
	*p++ = *s++;
	*p = *s;
	pk->offset += 3;
	return 0;
}

static inline int as_pack_int32(as_packer * pk, unsigned char type, uint32_t val) {
	if (pk->offset + 5 > pk->capacity) {
		if (as_pack_resize(pk, 5)) {
			return -1;
		}
	}
	uint32_t swapped = cf_swap_to_be32(val);
	unsigned char* p = pk->buffer + pk->offset;
	*p++ = type;
	memcpy(p, &swapped, 4);
	pk->offset += 5;
	return 0;
}

static inline int as_pack_int64(as_packer * pk, unsigned char type, uint64_t val) {
	if (pk->offset + 9 > pk->capacity) {
		if (as_pack_resize(pk, 9)) {
			return -1;
		}
	}
	uint64_t swapped = cf_swap_to_be64(val);
	unsigned char* p = pk->buffer + pk->offset;
	*p++ = type;
	memcpy(p, &swapped, 8);
	pk->offset += 9;
	return 0;
}

static inline int as_pack_boolean(as_packer * pk, as_boolean * b)
{
	return as_pack_byte(pk, (as_boolean_get(b) == true)? 0xc3 : 0xc2);
}

static int as_pack_integer(as_packer * pk, as_integer * i)
{
	int64_t val = as_integer_get(i);
	
	if (val >= 0) {
		if (val < 128) {
			return as_pack_byte(pk, (uint8_t)val);
		}
		
		if (val < 256) {
			return as_pack_int8(pk, 0xcc, (uint8_t)val);
		}
		
		if (val < 65536) {
			return as_pack_int16(pk, 0xcd, (uint16_t)val);
		}
		
		if (val < 4294967296) {
			return as_pack_int32(pk, 0xce, (uint32_t)val);
		}
		return as_pack_int64(pk, 0xcf, (uint64_t)val);
	}
	else {
		if (val >= -32) {
			return as_pack_byte(pk, (uint8_t)(0xe0 | (val + 32)));
		}
		
		if (val >= -128) {
			return as_pack_int8(pk, 0xd0, (uint8_t)val);
		}
		
		if (val >= -32768) {
			return as_pack_int16(pk, 0xd1, (uint16_t)val);
		}
		
		if (val >= 0x80000000) {
			return as_pack_int32(pk, 0xd2, (uint32_t)val);
		}
		return as_pack_int64(pk, 0xd3, (uint64_t)val);
	}
}

static int as_pack_byte_array_header(as_packer * pk, uint32_t length, uint8_t type)
{
	length++;  // Account for extra aerospike type.
	
	int rc;

	if (length < 32) {
		rc = as_pack_byte(pk, (uint8_t)(0xa0 | length));
	} else if (length < 65536) {
		rc = as_pack_int16(pk, 0xda, (uint16_t)length);
	} else {
		rc = as_pack_int32(pk, 0xdb, length);
	}
	
	if (rc == 0) {
		rc = as_pack_byte(pk, type);
	}
	return rc;
}

static int as_pack_string(as_packer * pk, as_string * s)
{
	uint32_t length = (uint32_t)as_string_len(s);
	int rc = as_pack_byte_array_header(pk, length, AS_BYTES_STRING);
	
	if (rc == 0) {
		rc = as_pack_append(pk, (unsigned char*)s->value, length);
	}
	return rc;
}

static int as_pack_bytes(as_packer * pk, as_bytes * b)
{
	int rc = as_pack_byte_array_header(pk, b->size, b->type);
	
	if (rc == 0) {
		rc = as_pack_append(pk, b->value, b->size);
	}
	return rc;
}

static bool as_pack_list_foreach(as_val * val, void * udata)
{
	as_packer* pk = (as_packer*)udata;
	return as_pack_val(pk, val) == 0;
}

static int as_pack_list(as_packer * pk, as_list * l)
{
	uint32_t size = as_list_size(l);
	int rc;
	
	if (size < 16) {
		rc = as_pack_byte(pk, (uint8_t)(0x90 | size));
	}
	else if (size < 65536) {
		rc = as_pack_int16(pk, 0xdc, (uint16_t)size);
	} else {
		rc = as_pack_int32(pk, 0xdd, size);
	}
				
	if (rc == 0) {
		rc = as_list_foreach(l, as_pack_list_foreach, pk) == true ? 0 : 1;
	}
	return rc;
}

static bool as_pack_map_foreach(const as_val * key, const as_val * val, void * udata)
{
	as_packer* pk = (as_packer*)udata;
	int rc = as_pack_val(pk, (as_val*)key);
	
	if (rc == 0) {
		rc = as_pack_val(pk, (as_val*)val);
	}
	return rc == 0;
}

static int as_pack_map(as_packer * pk, as_map * m)
{
	uint32_t size = as_map_size(m);
	int rc;
	
	if (size < 16) {
		rc = as_pack_byte(pk, (uint8_t)(0x80 | size));
	} else if (size < 65536) {
		rc = as_pack_int16(pk, 0xde, (uint16_t)size);
	} else {
		rc = as_pack_int32(pk, 0xdf, size);
	}
	
	if (rc == 0) {
		rc = as_map_foreach(m, as_pack_map_foreach, pk) == true ? 0 : 1;
	}
	return rc;
}

static int as_pack_rec(as_packer * pk, as_rec * r)
{
	return 1;
}

static int as_pack_pair(as_packer * pk, as_pair * p)
{
	unsigned char v = (unsigned char)(0x90 | 2);
	int rc = as_pack_append(pk, &v, 1);
		
	if (rc == 0) {
		rc = as_pack_val(pk, as_pair_1(p));
		
		if (rc == 0) {
			rc = as_pack_val(pk, as_pair_2(p));
		}
	}
	return rc;
}

int as_pack_val(as_packer * pk, as_val * val)
{
	int rc = 0;

	if (val == NULL) {
		rc = 1;
	}
	else {	
		switch (as_val_type(val)) {
			case AS_NIL : 
				rc = as_pack_byte(pk, 0xc0);
				break;
			case AS_BOOLEAN : 
				rc = as_pack_boolean(pk, (as_boolean *) val);
				break;
			case AS_INTEGER : 
				rc = as_pack_integer(pk, (as_integer *) val);
				break;
			case AS_STRING : 
				rc = as_pack_string(pk, (as_string *) val);
				break;
			case AS_BYTES : 
				rc = as_pack_bytes(pk, (as_bytes *) val);
				break;
			case AS_LIST : 
				rc = as_pack_list(pk, (as_list *) val);
				break;
			case AS_MAP : 
				rc = as_pack_map(pk, (as_map *) val);
				break;
			case AS_REC : 
				rc = as_pack_rec(pk, (as_rec *) val);
				break;
			case AS_PAIR : 
				rc = as_pack_pair(pk, (as_pair *) val);
				break;
			default : 
				rc = 2;
				break;
		}
	}
	return rc;
}

/******************************************************************************
 * UNPACK FUNCTIONS
 ******************************************************************************/

static inline uint16_t as_extract_uint16(as_unpacker * pk)
{
	uint16_t v = *(uint16_t*)(pk->buffer + pk->offset);
	uint16_t swapped = cf_swap_from_be16(v);
	pk->offset += 2;
	return swapped;
}

static inline uint32_t as_extract_uint32(as_unpacker * pk)
{
	uint32_t v = *(uint32_t*)(pk->buffer + pk->offset);
	uint32_t swapped = cf_swap_from_be32(v);
	pk->offset += 4;
	return swapped;
}

static inline uint64_t as_extract_uint64(as_unpacker * pk)
{
	uint64_t v = *(uint64_t*)(pk->buffer + pk->offset);
	uint64_t swapped = cf_swap_from_be64(v);
	pk->offset += 8;
	return swapped;
}

static inline float as_extract_float(as_unpacker * pk)
{
	uint32_t v = *(uint32_t*)(pk->buffer + pk->offset);
	uint32_t swapped = cf_swap_from_be32(v);
	pk->offset += 4;
	return *(float*)&swapped;
}

static inline double as_extract_double(as_unpacker * pk)
{
	uint64_t v = *(uint64_t*)(pk->buffer + pk->offset);
	uint64_t swapped = cf_swap_from_be64(v);
	pk->offset += 8;
	return *(double*)&swapped;
}

static inline int as_unpack_nil(as_val ** v)
{
	*v = (as_val*) &as_nil;
	return 0;
}

static inline int as_unpack_boolean(bool b, as_val ** v)
{
	// Aerospike does not support boolean, so we convert it to integer.
	*v = (as_val*) as_integer_new(b == true ? 1 : 0);
	return 0;
}

static inline int as_unpack_integer(int64_t i, as_val ** v)
{
	*v = (as_val*) as_integer_new(i);
	return 0;
}

static int as_unpack_blob(as_unpacker * pk, int size, as_val ** val)
{
	unsigned char type = pk->buffer[pk->offset++];
	size--;
	
	if (type == AS_BYTES_STRING) {
		char* v = strndup((char*)pk->buffer + pk->offset, size);
		*val = (as_val*) as_string_new(v, true);
	}
	else {
		unsigned char* buf = cf_malloc(size);
		memcpy(buf, pk->buffer + pk->offset, size);
		as_bytes *b = as_bytes_new_wrap(buf, size, true);
		if (b) {
			b->type = (as_bytes_type) type;
		}
		*val = (as_val*)b;
	}
	pk->offset += size;
	return 0;
}

static int as_unpack_list(as_unpacker * pk, int size, as_val ** val)
{
	as_arraylist* list = as_arraylist_new(size, 8);
	
	for (int i = 0; i < size; i++) {
		as_val* v = 0;
		as_unpack_val(pk, &v);
		
		if (v) {
			as_arraylist_set(list, i, v);
		}
	}
	*val = (as_val*)list;
	return 0;
}

static int as_unpack_map(as_unpacker * pk, int size, as_val ** val)
{
	as_hashmap* map = as_hashmap_new(32);
	
	for (int i = 0; i < size; i++) {
		as_val* k = 0;
		as_val* v = 0;
		as_unpack_val(pk, &k);
		as_unpack_val(pk, &v);
		
		if (k && v) {
			as_hashmap_set(map, k, v);
 		}
	}
	*val = (as_val*)map;
	return 0;
}

int as_unpack_val(as_unpacker * pk, as_val ** val)
{
	uint8_t type = pk->buffer[pk->offset++];
	
	switch (type) {
		case 0xc0: { // nil
			return as_unpack_nil(val);
		}
			
		case 0xc3: { // boolean true
			return as_unpack_boolean(true, val);
		}
			
		case 0xc2: { // boolean false
			return as_unpack_boolean(false, val);
		}
			
		case 0xca: { // float
			float v = as_extract_float(pk);
			// Convert to integer because float is not currently supported.
			return as_unpack_integer((int64_t)v, val);
		}
			
		case 0xcb: { // double
			double v = as_extract_double(pk);
			// Convert to integer because double is not currently supported.
			return as_unpack_integer((int64_t)v, val);
		}
		
		case 0xd0:   // signed 8 bit integer
		case 0xcc: { // unsigned 8 bit integer
			int8_t v = pk->buffer[pk->offset++];
			return as_unpack_integer(v, val);
		}
		
		case 0xd1:   // signed 16 bit integer
		case 0xcd: { // unsigned 16 bit integer
			int16_t v = as_extract_uint16(pk);
			return as_unpack_integer(v, val);
		}
		
		case 0xd2:   // signed 32 bit integer
		case 0xce: { // unsigned 32 bit integer
			int32_t v = as_extract_uint32(pk);
			return as_unpack_integer(v, val);
		}
		
		case 0xd3:   // signed 64 bit integer
		case 0xcf: { // unsigned 64 bit integer
			int64_t v = as_extract_uint64(pk);
			return as_unpack_integer(v, val);
		}
			
		case 0xda: { // raw bytes with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			return as_unpack_blob(pk, length, val);
		}
			
		case 0xdb: { // raw bytes with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			return as_unpack_blob(pk, length, val);
		}
			
		case 0xdc: { // list with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			return as_unpack_list(pk, length, val);
		}
			
		case 0xdd: { // list with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			return as_unpack_list(pk, length, val);
		}
			
		case 0xde: { // map with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			return as_unpack_map(pk, length, val);
		}
			
		case 0xdf: { // map with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			return as_unpack_map(pk, length, val);
		}
			
		default: {
			if ((type & 0xe0) == 0xa0) { // raw bytes with 8 bit combined header
				return as_unpack_blob(pk, type & 0x1f, val);
			}
			
			if ((type & 0xf0) == 0x80) { // map with 8 bit combined header
				return as_unpack_map(pk, type & 0x0f, val);
			}
			
			if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
				return as_unpack_list(pk, type & 0x0f, val);
			}
			
			if (type < 0x80) { // 8 bit combined unsigned integer
				return as_unpack_integer(type, val);
			}
			
			if (type >= 0xe0) { // 8 bit combined signed integer
				return as_unpack_integer(type - 0xe0 - 32, val);
			}
			return 2;
		}
	}
}
