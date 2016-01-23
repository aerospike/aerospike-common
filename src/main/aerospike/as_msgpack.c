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
#include <aerospike/as_serializer.h>
#include <aerospike/as_types.h>
#include <citrusleaf/cf_byte_order.h>

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
	if (pk->buffer) {
		if (pk->offset + length > pk->capacity) {
			if (as_pack_resize(pk, length)) {
				return -1;
			}
		}
		memcpy(pk->buffer + pk->offset, src, length);
	}
	pk->offset += length;
	return 0;
}

static inline int as_pack_byte(as_packer * pk, uint8_t val) {
	if (pk->buffer) {
		if (pk->offset + 1 > pk->capacity) {
			if (as_pack_resize(pk, 1)) {
				return -1;
			}
		}
		*(pk->buffer + pk->offset) = val;
	}
	pk->offset++;
	return 0;
}

static inline int as_pack_int8(as_packer * pk, unsigned char type, uint8_t val) {
	if (pk->buffer) {
		if (pk->offset + 2 > pk->capacity) {
			if (as_pack_resize(pk, 2)) {
				return -1;
			}
		}
		unsigned char* p = pk->buffer + pk->offset;
		*p++ = type;
		*p = val;
	}
	pk->offset += 2;
	return 0;
}

static inline int as_pack_int16(as_packer * pk, unsigned char type, uint16_t val) {
	if (pk->buffer) {
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
	}
	pk->offset += 3;
	return 0;
}

static inline int as_pack_int32(as_packer * pk, unsigned char type, uint32_t val) {
	if (pk->buffer) {
		if (pk->offset + 5 > pk->capacity) {
			if (as_pack_resize(pk, 5)) {
				return -1;
			}
		}
		uint32_t swapped = cf_swap_to_be32(val);
		unsigned char* p = pk->buffer + pk->offset;
		*p++ = type;
		memcpy(p, &swapped, 4);
	}
	pk->offset += 5;
	return 0;
}

static inline int as_pack_int64(as_packer * pk, unsigned char type, uint64_t val) {
	if (pk->buffer) {
		if (pk->offset + 9 > pk->capacity) {
			if (as_pack_resize(pk, 9)) {
				return -1;
			}
		}
		uint64_t swapped = cf_swap_to_be64(val);
		unsigned char* p = pk->buffer + pk->offset;
		*p++ = type;
		memcpy(p, &swapped, 8);
	}
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
		
		if (val >= -0x80000000L) {
			return as_pack_int32(pk, 0xd2, (uint32_t)val);
		}
		return as_pack_int64(pk, 0xd3, (uint64_t)val);
	}
}

static inline int as_pack_double(as_packer * pk, as_double* d)
{
	double val = as_double_get(d);
	return as_pack_int64(pk, 0xcb, *(uint64_t*)&val);
}

static int as_pack_byte_array_header(as_packer * pk, uint32_t length, uint8_t type)
{
	length++;  // Account for extra aerospike type.
	
	int rc;

	// Continue to pack byte arrays as strings until all servers/clients
	// have been upgraded to handle new message pack binary type.
	if (length < 32) {
		rc = as_pack_byte(pk, (uint8_t)(0xa0 | length));
	} else if (length < 65536) {
		rc = as_pack_int16(pk, 0xda, (uint16_t)length);
	} else {
		rc = as_pack_int32(pk, 0xdb, length);
	}
	
	// TODO: Replace with this code after all servers/clients
	// have been upgraded to handle new message pack binary type.
	/*
	 if (length < 32) {
		rc = as_pack_byte(pk, (uint8_t)(0xa0 | length));
	 } else if (length < 256) {
		rc = as_pack_int8(pk, 0xc4, (uint8_t)length);
	 } else if (length < 65536) {
		rc = as_pack_int16(pk, 0xc5, (uint16_t)length);
	 } else {
		rc = as_pack_int32(pk, 0xc6, length);
	 }
	 */

	if (rc == 0) {
		rc = as_pack_byte(pk, type);
	}
	return rc;
}

static int as_pack_string(as_packer * pk, as_string * s)
{
	uint32_t length = (uint32_t)as_string_len(s);
	uint32_t size = length + 1;
	int rc;
		
	if (size < 32) {
		rc = as_pack_byte(pk, (uint8_t)(0xa0 | size));
	// TODO: Enable this code after all servers/clients
	// have been upgraded to handle new message pack binary type.
	//} else if (size < 255) {
	//	rc = as_pack_int8(pk, 0xd9, (uint8_t)size);
	} else if (size < 65536) {
		rc = as_pack_int16(pk, 0xda, (uint16_t)size);
	} else {
		rc = as_pack_int32(pk, 0xdb, size);
	}
		
	if (rc == 0) {
		rc = as_pack_byte(pk, AS_BYTES_STRING);
	}

	if (rc == 0) {
		rc = as_pack_append(pk, (unsigned char*)s->value, length);
	}
	return rc;
}

static int as_pack_geojson(as_packer * pk, as_geojson * s)
{
	uint32_t length = (uint32_t)as_geojson_len(s);
	int rc = as_pack_byte_array_header(pk, length, AS_BYTES_GEOJSON);
	
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

int as_pack_val(as_packer * pk, const as_val * val)
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
			case AS_DOUBLE :
				rc = as_pack_double(pk, (as_double *) val);
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
			case AS_GEOJSON : 
				rc = as_pack_geojson(pk, (as_geojson *) val);
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

static inline int as_unpack_integer_val(int64_t i, as_val ** v)
{
	*v = (as_val*) as_integer_new(i);
	return 0;
}

static inline int as_unpack_double_val(double d, as_val ** v)
{
	*v = (as_val*) as_double_new(d);
	return 0;
}

static int as_unpack_blob(as_unpacker * pk, int size, as_val ** val)
{
	unsigned char type = pk->buffer[pk->offset++];
	size--;
	
	if (type == AS_BYTES_STRING) {
		char* v = cf_strndup((const char *)pk->buffer + pk->offset, size);
		*val = (as_val*) as_string_new(v, true);
	}
	else if (type == AS_BYTES_GEOJSON) {
		char* v = cf_strndup((const char *)pk->buffer + pk->offset, size);
		*val = (as_val*) as_geojson_new(v, true);
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
	as_hashmap* map = as_hashmap_new(size > 32 ? size : 32);
	
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
			return as_unpack_double_val(v, val);
		}
			
		case 0xcb: { // double
			double v = as_extract_double(pk);
			return as_unpack_double_val(v, val);
		}
		
		case 0xd0: { // signed 8 bit integer
			int8_t v = pk->buffer[pk->offset++];
			return as_unpack_integer_val(v, val);
		}
		case 0xcc: { // unsigned 8 bit integer
			uint8_t v = pk->buffer[pk->offset++];
			return as_unpack_integer_val(v, val);
		}
		
		case 0xd1: { // signed 16 bit integer
			int16_t v = as_extract_uint16(pk);
			return as_unpack_integer_val(v, val);
		}
		case 0xcd: { // unsigned 16 bit integer
			uint16_t v = as_extract_uint16(pk);
			return as_unpack_integer_val(v, val);
		}
		
		case 0xd2: { // signed 32 bit integer
			int32_t v = as_extract_uint32(pk);
			return as_unpack_integer_val(v, val);
		}
		case 0xce: { // unsigned 32 bit integer
			uint32_t v = as_extract_uint32(pk);
			return as_unpack_integer_val(v, val);
		}
		
		case 0xd3: { // signed 64 bit integer
			int64_t v = as_extract_uint64(pk);
			return as_unpack_integer_val(v, val);
		}
		case 0xcf: { // unsigned 64 bit integer
			uint64_t v = as_extract_uint64(pk);
			return as_unpack_integer_val(v, val);
		}
		
		case 0xc4:
		case 0xd9: { // string/raw bytes with 8 bit header
			uint8_t length = pk->buffer[pk->offset++];
			return as_unpack_blob(pk, length, val);
		}

		case 0xc5:
		case 0xda: { // string/raw bytes with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			return as_unpack_blob(pk, length, val);
		}
			
		case 0xc6:
		case 0xdb: { // string/raw bytes with 32 bit header
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
				return as_unpack_integer_val(type, val);
			}
			
			if (type >= 0xe0) { // 8 bit combined signed integer
				return as_unpack_integer_val(type - 0xe0 - 32, val);
			}
			return 2;
		}
	}
}

/******************************************************************************
 * Pack direct functions
 ******************************************************************************/

int as_pack_list_header(as_packer *pk, uint32_t ele_count)
{
	int rc;
	if (ele_count < 16) {
		rc = as_pack_byte(pk, (uint8_t)(0x90 | ele_count));
	}
	else if (ele_count < 65536) {
		rc = as_pack_int16(pk, 0xdc, (uint16_t)ele_count);
	}
	else {
		rc = as_pack_int32(pk, 0xdd, ele_count);
	}
	return rc;
}

uint32_t as_pack_list_header_get_size(uint32_t ele_count)
{
	if (ele_count < 16) {
		return 1;
	}
	else if (ele_count < 65536) {
		return 3;
	}
	else {
		return 5;
	}
}

/******************************************************************************
 * Unpack direct functions
 ******************************************************************************/

/**
 * Get size of list with ele_count elements.
 * Assume header already extracted.
 * @return -1 on failure
 */
static int64_t as_unpack_list_elements_size(as_unpacker *pk, uint32_t ele_count)
{
	int64_t total = 0;
	for (uint32_t i = 0; i < ele_count; i++) {
		int64_t ret = as_unpack_size(pk);
		if (ret < 0) {
			return -1;
		}
		total += ret;
	}
	return total;
}

/**
 * Get size of map with ele_count elements.
 * Assume header already extracted.
 * @return -1 on failure
 */
static int64_t as_unpack_map_elements_size(as_unpacker *pk, uint32_t ele_count)
{
	int64_t total = 0;
	for (uint32_t i = 0; i < ele_count; i++) {
		int64_t ret = as_unpack_size(pk);
		if (ret < 0) {
			return -1;
		}
		total += ret;

		ret = as_unpack_size(pk);
		if (ret < 0) {
			return -1;
		}
		total += ret;
	}
	return total;
}

static inline as_val_t bytes_internal_type_to_as_val_t(uint8_t type)
{
	if (type == AS_BYTES_STRING) {
		return AS_STRING;
	}
	else if (type == AS_BYTES_GEOJSON) {
		return AS_GEOJSON;
	}
	// All other types are considered AS_BYTES.
	return AS_BYTES;
}

as_val_t as_unpack_peek_type(const as_unpacker *pk)
{
	uint8_t type = pk->buffer[pk->offset];

	switch (type) {
	case 0xc0:	// nil
		return AS_NIL;

	case 0xc3:	// boolean true
	case 0xc2:	// boolean false
		return AS_BOOLEAN;

	case 0xd0:	// signed 8 bit integer
	case 0xcc:	// unsigned 8 bit integer
	case 0xd1:	// signed 16 bit integer
	case 0xcd:	// unsigned 16 bit integer
	case 0xd2:	// signed 32 bit integer
	case 0xce:	// unsigned 32 bit integer
	case 0xd3:	// signed 64 bit integer
	case 0xcf:	// unsigned 64 bit integer
		return AS_INTEGER;

	case 0xca:	// float
	case 0xcb:	// double
		return AS_DOUBLE;

	case 0xc4:
	case 0xd9: { // string/raw bytes with 8 bit header
		uint8_t type1 = pk->buffer[pk->offset + 2];
		return bytes_internal_type_to_as_val_t(type1);
	}

	case 0xc5:
	case 0xda: { // string/raw bytes with 16 bit header
		uint8_t type1 = pk->buffer[pk->offset + 3];
		return bytes_internal_type_to_as_val_t(type1);
	}

	case 0xc6:
	case 0xdb: { // string/raw bytes with 32 bit header
		uint8_t type1 = pk->buffer[pk->offset + 5];
		return bytes_internal_type_to_as_val_t(type1);
	}

	case 0xdc:	// list with 16 bit header
	case 0xdd:	// list with 32 bit header
		return AS_LIST;

	case 0xde:	// map with 16 bit header
	case 0xdf:	// map with 32 bit header
		return AS_MAP;

	default:
		if ((type & 0xe0) == 0xa0) { // raw bytes with 8 bit combined header
			uint8_t type1 = pk->buffer[pk->offset + 1];
			return bytes_internal_type_to_as_val_t(type1);
		}

		if ((type & 0xf0) == 0x80) { // map with 8 bit combined header
			return AS_MAP;
		}

		if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
			return AS_LIST;
		}

		if (type < 0x80) { // 8 bit combined unsigned integer
			return AS_INTEGER;
		}

		if (type >= 0xe0) { // 8 bit combined signed integer
			return AS_INTEGER;
		}

		break;
	}

	return AS_UNDEF;
}

as_val_t as_unpack_buf_peek_type(const uint8_t *buf, uint32_t size)
{
	const as_unpacker pk = {
			.buffer = buf,
			.offset = 0,
			.length = size,
	};

	return as_unpack_peek_type(&pk);
}

int64_t as_unpack_size(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];

	switch (type) {
		case 0xc0:	// nil
		case 0xc3:	// boolean true
		case 0xc2:	// boolean false
			return 1;

		case 0xd0:	// signed 8 bit integer
		case 0xcc:	// unsigned 8 bit integer
			pk->offset++;
			return 1 + 1;

		case 0xd1:	// signed 16 bit integer
		case 0xcd:	// unsigned 16 bit integer
			pk->offset += 2;
			return 1 + 2;

		case 0xca:	// float
		case 0xd2:	// signed 32 bit integer
		case 0xce:	// unsigned 32 bit integer
			pk->offset += 4;
			return 1 + 4;

		case 0xcb:	// double
		case 0xd3:	// signed 64 bit integer
		case 0xcf:	// unsigned 64 bit integer
			pk->offset += 8;
			return 1 + 8;

		case 0xc4:
		case 0xd9: { // string/raw bytes with 8 bit header
			uint8_t length = pk->buffer[pk->offset++];
			pk->offset += length;
			return 1 + 1 + length;
		}

		case 0xc5:
		case 0xda: { // string/raw bytes with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			pk->offset += length;
			return 1 + 2 + length;
		}

		case 0xc6:
		case 0xdb: { // string/raw bytes with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			pk->offset += length;
			return 1 + 4 + length;
		}

		case 0xdc: { // list with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			int64_t ret = as_unpack_list_elements_size(pk, length);
			if (ret < 0) {
				return -1;
			}
			return 1 + 2 + ret;
		}

		case 0xdd: { // list with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			int64_t ret = as_unpack_list_elements_size(pk, length);
			if (ret < 0) {
				return -1;
			}
			return 1 + 4 + ret;
		}

		case 0xde: { // map with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			int64_t ret = as_unpack_map_elements_size(pk, length);
			if (ret < 0) {
				return -1;
			}
			return 1 + 2 + ret;
		}

		case 0xdf: { // map with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			int64_t ret = as_unpack_map_elements_size(pk, length);
			if (ret < 0) {
				return -1;
			}
			return 1 + 4 + ret;
		}

		default:
			break;
	}

	if ((type & 0xe0) == 0xa0) { // raw bytes with 8 bit combined header
		int length = type & 0x1f;
		pk->offset += length;
		return 1 + length;
	}

	if ((type & 0xf0) == 0x80) { // map with 8 bit combined header
		int64_t ret = as_unpack_map_elements_size(pk, type & 0x0f);
		if (ret < 0) {
			return -1;
		}
		return 1 + ret;
	}

	if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
		int64_t ret = as_unpack_list_elements_size(pk, type & 0x0f);
		if (ret < 0) {
			return -1;
		}
		return 1 + ret;
	}

	if (type < 0x80) { // 8 bit combined unsigned integer
		return 1;
	}

	if (type >= 0xe0) { // 8 bit combined signed integer
		return 1;
	}

	return -1;
}

int64_t as_unpack_blob_size(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
		case 0xc4:
		case 0xd9: { // string/raw bytes with 8 bit header
			if (size < 1) {
				return -2;
			}

			uint8_t length = pk->buffer[pk->offset++];
			return length;
		}

		case 0xc5:
		case 0xda: { // string/raw bytes with 16 bit header
			if (size < 2) {
				return -3;
			}

			uint16_t length = as_extract_uint16(pk);
			return length;
		}

		case 0xc6:
		case 0xdb: { // string/raw bytes with 32 bit header
			if (size < 4) {
				return -4;
			}

			uint32_t length = as_extract_uint32(pk);
			return length;
		}

		default: {
			if ((type & 0xe0) == 0xa0) { // raw bytes with 8 bit combined header
				return type & 0x1f;
			}
			break;
		}
	}

	return -5;
}

int as_unpack_uint64(as_unpacker *pk, uint64_t *i)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
		case 0xd0: { // signed 8 bit integer
			if (size < 1) {
				return -1;
			}
			int8_t v = pk->buffer[pk->offset++];
			*i = v;
			break;
		}
		case 0xcc: { // unsigned 8 bit integer
			if (size < 1) {
				return -2;
			}
			uint8_t v = pk->buffer[pk->offset++];
			*i = v;
			break;
		}

		case 0xd1: { // signed 16 bit integer
			if (size < 2) {
				return -3;
			}
			int16_t v = as_extract_uint16(pk);
			*i = v;
			break;
		}
		case 0xcd: { // unsigned 16 bit integer
			if (size < 2) {
				return -4;
			}
			uint16_t v = as_extract_uint16(pk);
			*i = v;
			break;
		}

		case 0xd2: { // signed 32 bit integer
			if (size < 4) {
				return -5;
			}
			int32_t v = as_extract_uint32(pk);
			*i = v;
			break;
		}
		case 0xce: { // unsigned 32 bit integer
			if (size < 4) {
				return -6;
			}
			uint32_t v = as_extract_uint32(pk);
			*i = v;
			break;
		}

		case 0xd3: { // signed 64 bit integer
			if (size < 8) {
				return -7;
			}
			int64_t v = as_extract_uint64(pk);
			*i = v;
			break;
		}
		case 0xcf: { // unsigned 64 bit integer
			if (size < 8) {
				return -8;
			}
			uint64_t v = as_extract_uint64(pk);
			*i = v;
			break;
		}
		default: {
			if (type < 0x80) { // 8 bit combined unsigned integer
				*i = type;
				break;
			}

			if (type >= 0xe0) { // 8 bit combined signed integer
				int8_t v = type - 0xe0 - 32;
				*i = v;
				break;
			}

			return -9;
		}
	}

	return 0;
}

int as_unpack_int64(as_unpacker *pk, int64_t *i)
{
	return as_unpack_uint64(pk, (uint64_t *)i);
}

int as_unpack_double(as_unpacker *pk, double *x)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xca: { // float
		if (size < 4) {
			return -2;
		}
		*x = (double)as_extract_float(pk);
		break;
	}
	case 0xcb: { // double
		if (size < 8) {
			return -3;
		}
		*x = as_extract_double(pk);
		break;
	}
	default:
		return -4;
	}

	return 0;
}

int64_t as_unpack_buf_list_element_count(const uint8_t *buf, uint32_t size)
{
	as_unpacker pk = {
			.buffer = buf,
			.offset = 0,
			.length = size,
	};

	return as_unpack_list_header_element_count(&pk);
}

int64_t as_unpack_list_header_element_count(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
		case 0xdc: { // list with 16 bit header
			if (size < 2) {
				return -2;
			}
			return as_extract_uint16(pk);
		}
		case 0xdd: { // list with 32 bit header
			if (size < 4) {
				return -3;
			}
			return as_extract_uint32(pk);
		}
		default:
			break;
	}

	if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
		return type & 0x0f;
	}

	return -4;
}
