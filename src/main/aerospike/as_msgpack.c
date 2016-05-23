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
 * INTERNAL TYPEDEFS & CONSTANTS
 ******************************************************************************/

#define MSGPACK_COMPARE_MAX_DEPTH	256
#define MSGPACK_PARSE_MEMBLOCK_STATE_COUNT	256

typedef struct msgpack_parse_state_s {
	uint32_t len1;
	uint32_t len2;
	uint32_t len;
	uint32_t index;
	uint8_t map_pair;
	as_val_t type;
	msgpack_compare_t default_compare_type;
} msgpack_parse_state;

typedef struct msgpack_parse_memblock_s {
	struct msgpack_parse_memblock_s *prev;
	msgpack_parse_state buffer[MSGPACK_PARSE_MEMBLOCK_STATE_COUNT];
	size_t count;
} msgpack_parse_memblock;

#define MSGPACK_COMPARE_RET_LESS_OR_GREATER(arg1, arg2) { \
	if (arg1 < arg2) { \
		return MSGPACK_COMPARE_LESS; \
	} \
	if (arg1 > arg2) { \
		return MSGPACK_COMPARE_GREATER; \
	} \
}

/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/

// msgpack_parse
static msgpack_parse_memblock *msgpack_parse_memblock_create(msgpack_parse_memblock *prev);
static void msgpack_parse_memblock_destroy(msgpack_parse_memblock *block);
static msgpack_parse_state *msgpack_parse_memblock_next(msgpack_parse_memblock **block);
static inline bool msgpack_parse_memblock_has_prev(msgpack_parse_memblock *block);
static msgpack_parse_state *msgpack_parse_memblock_prev(msgpack_parse_memblock **block);
static bool msgpack_parse_state_list_cmp_init(msgpack_parse_state *state, as_unpacker *pk1, as_unpacker *pk2);
static bool msgpack_parse_state_list_size_init(msgpack_parse_state *state, as_unpacker *pk);
static bool msgpack_parse_state_map_cmp_init(msgpack_parse_state *state, as_unpacker *pk1, as_unpacker *pk2);
static bool msgpack_parse_state_map_size_init(msgpack_parse_state *state, as_unpacker *pk);

// msgpack_compare
static inline msgpack_compare_t msgpack_compare_int(as_unpacker *pk1, as_unpacker *pk2);
static inline msgpack_compare_t msgpack_compare_double(as_unpacker *pk1, as_unpacker *pk2);
static inline int64_t msgpack_get_blob_len(as_unpacker *pk);
static msgpack_compare_t msgpack_compare_blob_internal(as_unpacker *pk1, int64_t len1, as_unpacker *pk2, int64_t len2);
static inline msgpack_compare_t msgpack_compare_blob(as_unpacker *pk1, as_unpacker *pk2);
static inline msgpack_compare_t msgpack_compare_int64_t(int64_t x1, int64_t x2);
static bool msgpack_skip(as_unpacker *pk, size_t n);
static bool msgpack_compare_unwind(as_unpacker *pk1, as_unpacker *pk2, msgpack_parse_memblock *block, msgpack_parse_state *state);
static msgpack_compare_t msgpack_compare_list(as_unpacker *pk1, as_unpacker *pk2, size_t depth);
static msgpack_compare_t msgpack_compare_map(as_unpacker *pk1, as_unpacker *pk2, size_t depth);
static inline msgpack_compare_t msgpack_peek_compare_type(const as_unpacker *pk1, const as_unpacker *pk2, as_val_t *type);
static msgpack_compare_t msgpack_compare_non_recursive(as_unpacker *pk1, as_unpacker *pk2, msgpack_parse_memblock *block, msgpack_parse_state *state);
static inline msgpack_compare_t msgpack_compare_type(as_unpacker *pk1, as_unpacker *pk2, as_val_t type, size_t depth);
static inline msgpack_compare_t msgpack_compare_internal(as_unpacker *pk1, as_unpacker *pk2, size_t depth);

// unpack direct
static int64_t as_unpack_list_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth);
static int64_t as_unpack_map_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth);
static inline as_val_t bytes_internal_type_to_as_val_t(uint8_t type);
static int64_t as_unpack_size_non_recursive(as_unpacker *pk, msgpack_parse_memblock *block, msgpack_parse_state *state);
static inline int64_t as_unpack_size_internal(as_unpacker *pk, uint32_t depth);

/******************************************************************************
 * MSGPACK_PARSE FUNCTIONS
 ******************************************************************************/

static msgpack_parse_memblock *msgpack_parse_memblock_create(msgpack_parse_memblock *prev)
{
	msgpack_parse_memblock *p = malloc(sizeof(msgpack_parse_memblock));
	p->prev = prev;
	p->count = 0;
	return p;
}

static void msgpack_parse_memblock_destroy(msgpack_parse_memblock *block)
{
	while (block) {
		msgpack_parse_memblock *p = block;
		block = block->prev;
		free(p);
	}
}

static msgpack_parse_state *msgpack_parse_memblock_next(msgpack_parse_memblock **block)
{
	msgpack_parse_memblock *ptr = *block;

	if (ptr->count >= MSGPACK_PARSE_MEMBLOCK_STATE_COUNT) {
		ptr = msgpack_parse_memblock_create(ptr);
		*block = ptr;
	}

	return &ptr->buffer[ptr->count++];
}

static inline bool msgpack_parse_memblock_has_prev(msgpack_parse_memblock *block)
{
	if (block->prev || block->count > 1) {
		return true;
	}

	return false;
}

static msgpack_parse_state *msgpack_parse_memblock_prev(msgpack_parse_memblock **block)
{
	msgpack_parse_memblock *ptr = *block;

	if (ptr->count <= 1) {
		ptr = ptr->prev;
		free(*block);
		*block = ptr;
	}
	else {
		ptr->count--;
	}

	// No check for NULL ptr here, use has_prev to make sure it doesn't happen.

	return &ptr->buffer[ptr->count - 1];
}

static bool msgpack_parse_state_list_cmp_init(msgpack_parse_state *state, as_unpacker *pk1, as_unpacker *pk2)
{
	int64_t len1 = as_unpack_list_header_element_count(pk1);
	int64_t len2 = as_unpack_list_header_element_count(pk2);
	int64_t minlen = (len1 < len2) ? len1 : len2;

	if (minlen < 0) {
		return false;
	}

	state->len1 = (uint32_t)len1;
	state->len2 = (uint32_t)len2;
	state->index = 0;
	state->map_pair = 0;
	state->len = (uint32_t)minlen;
	state->type = AS_LIST;
	state->default_compare_type = msgpack_compare_int64_t(len1, len2);

	return true;
}

static bool msgpack_parse_state_list_size_init(msgpack_parse_state *state, as_unpacker *pk)
{
	int64_t len = as_unpack_list_header_element_count(pk);

	if (len < 0) {
		return false;
	}

	state->index = 0;
	state->map_pair = 0;
	state->len = (uint32_t)len;
	state->type = AS_LIST;

	return true;
}

static bool msgpack_parse_state_map_cmp_init(msgpack_parse_state *state, as_unpacker *pk1, as_unpacker *pk2)
{
	int64_t len1 = as_unpack_map_header_element_count(pk1);
	int64_t len2 = as_unpack_map_header_element_count(pk2);
	int64_t minlen = (len1 < len2) ? len1 : len2;

	if (minlen < 0) {
		return false;
	}

	state->len1 = (uint32_t)len1;
	state->len2 = (uint32_t)len2;
	state->index = 0;
	state->map_pair = 0;
	state->len = (uint32_t)minlen;
	state->type = AS_MAP;
	state->default_compare_type = msgpack_compare_int64_t(len1, len2);

	return true;
}

static bool msgpack_parse_state_map_size_init(msgpack_parse_state *state, as_unpacker *pk)
{
	int64_t len = as_unpack_map_header_element_count(pk);

	if (len < 0) {
		return false;
	}

	state->index = 0;
	state->map_pair = 0;
	state->len = (uint32_t)len;
	state->type = AS_MAP;

	return true;
}

/******************************************************************************
 * PACK FUNCTIONS
 ******************************************************************************/

static int as_pack_resize(as_packer *pk, int length)
{
	// Add current buffer to linked list and allocate a new buffer
	as_packer_buffer *entry = (as_packer_buffer *)cf_malloc(sizeof(as_packer_buffer));
	
	if (entry == 0) {
		return -1;
	}
	entry->buffer = pk->buffer;
	entry->length = pk->offset;
	entry->next = 0;
	
	int size = (length > pk->capacity) ? length : pk->capacity;
	pk->buffer = (unsigned char *)cf_malloc(size);
	
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

static inline int as_pack_append(as_packer *pk, const unsigned char *src, int length)
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

static inline int as_pack_byte(as_packer *pk, uint8_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 1 > pk->capacity) {
			if (! resize || as_pack_resize(pk, 1)) {
				return -1;
			}
		}
		*(pk->buffer + pk->offset) = val;
	}
	pk->offset++;
	return 0;
}

static inline int as_pack_int8(as_packer *pk, unsigned char type, uint8_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 2 > pk->capacity) {
			if (! resize || as_pack_resize(pk, 2)) {
				return -1;
			}
		}
		unsigned char *p = pk->buffer + pk->offset;
		*p++ = type;
		*p = val;
	}
	pk->offset += 2;
	return 0;
}

static inline int as_pack_int16(as_packer *pk, unsigned char type, uint16_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 3 > pk->capacity) {
			if (! resize || as_pack_resize(pk, 3)) {
				return -1;
			}
		}
		uint16_t swapped = cf_swap_to_be16(val);
		unsigned char *s = (unsigned char *)&swapped;
		unsigned char *p = pk->buffer + pk->offset;
		*p++ = type;
		*p++ = *s++;
		*p = *s;
	}
	pk->offset += 3;
	return 0;
}

static inline int as_pack_int32(as_packer *pk, unsigned char type, uint32_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 5 > pk->capacity) {
			if (! resize || as_pack_resize(pk, 5)) {
				return -1;
			}
		}
		uint32_t swapped = cf_swap_to_be32(val);
		unsigned char *p = pk->buffer + pk->offset;
		*p++ = type;
		memcpy(p, &swapped, 4);
	}
	pk->offset += 5;
	return 0;
}

static inline int as_pack_int64(as_packer *pk, unsigned char type, uint64_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 9 > pk->capacity) {
			if (! resize || as_pack_resize(pk, 9)) {
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

static inline int as_pack_boolean(as_packer *pk, as_boolean *b)
{
	return as_pack_byte(pk, (as_boolean_get(b) == true)? 0xc3 : 0xc2, true);
}

static int as_pack_integer(as_packer *pk, as_integer *i)
{
	int64_t val = as_integer_get(i);
	
	if (val >= 0) {
		if (val < 128) {
			return as_pack_byte(pk, (uint8_t)val, true);
		}
		
		if (val < 256) {
			return as_pack_int8(pk, 0xcc, (uint8_t)val, true);
		}
		
		if (val < 65536) {
			return as_pack_int16(pk, 0xcd, (uint16_t)val, true);
		}
		
		if (val < 4294967296) {
			return as_pack_int32(pk, 0xce, (uint32_t)val, true);
		}
		return as_pack_int64(pk, 0xcf, (uint64_t)val, true);
	}
	else {
		if (val >= -32) {
			return as_pack_byte(pk, (uint8_t)(0xe0 | (val + 32)), true);
		}
		
		if (val >= -128) {
			return as_pack_int8(pk, 0xd0, (uint8_t)val, true);
		}
		
		if (val >= -32768) {
			return as_pack_int16(pk, 0xd1, (uint16_t)val, true);
		}
		
		if (val >= -0x80000000L) {
			return as_pack_int32(pk, 0xd2, (uint32_t)val, true);
		}
		return as_pack_int64(pk, 0xd3, (uint64_t)val, true);
	}
}

static inline int as_pack_double(as_packer *pk, as_double *d)
{
	double val = as_double_get(d);
	return as_pack_int64(pk, 0xcb, *(uint64_t *)&val, true);
}

static int as_pack_byte_array_header(as_packer *pk, uint32_t length, uint8_t type)
{
	length++;  // Account for extra aerospike type.
	
	int rc;

	// Continue to pack byte arrays as strings until all servers/clients
	// have been upgraded to handle new message pack binary type.
	if (length < 32) {
		rc = as_pack_byte(pk, (uint8_t)(0xa0 | length), true);
	} else if (length < 65536) {
		rc = as_pack_int16(pk, 0xda, (uint16_t)length, true);
	} else {
		rc = as_pack_int32(pk, 0xdb, length, true);
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
		rc = as_pack_byte(pk, type, true);
	}
	return rc;
}

static int as_pack_string(as_packer *pk, as_string *s)
{
	uint32_t length = (uint32_t)as_string_len(s);
	uint32_t size = length + 1;
	int rc;
		
	if (size < 32) {
		rc = as_pack_byte(pk, (uint8_t)(0xa0 | size), true);
	// TODO: Enable this code after all servers/clients
	// have been upgraded to handle new message pack binary type.
	//} else if (size < 255) {
	//	rc = as_pack_int8(pk, 0xd9, (uint8_t)size);
	} else if (size < 65536) {
		rc = as_pack_int16(pk, 0xda, (uint16_t)size, true);
	} else {
		rc = as_pack_int32(pk, 0xdb, size, true);
	}
		
	if (rc == 0) {
		rc = as_pack_byte(pk, AS_BYTES_STRING, true);
	}

	if (rc == 0) {
		rc = as_pack_append(pk, (unsigned char*)s->value, length);
	}
	return rc;
}

static int as_pack_geojson(as_packer *pk, as_geojson *s)
{
	uint32_t length = (uint32_t)as_geojson_len(s);
	int rc = as_pack_byte_array_header(pk, length, AS_BYTES_GEOJSON);
	
	if (rc == 0) {
		rc = as_pack_append(pk, (unsigned char*)s->value, length);
	}
	return rc;
}

static int as_pack_bytes(as_packer *pk, as_bytes *b)
{
	int rc = as_pack_byte_array_header(pk, b->size, b->type);
	
	if (rc == 0) {
		rc = as_pack_append(pk, b->value, b->size);
	}
	return rc;
}

static bool as_pack_list_foreach(as_val *val, void *udata)
{
	as_packer *pk = (as_packer *)udata;
	return as_pack_val(pk, val) == 0;
}

static int as_pack_list(as_packer *pk, as_list *l)
{
	uint32_t size = as_list_size(l);
	int rc;
	
	if (size < 16) {
		rc = as_pack_byte(pk, (uint8_t)(0x90 | size), true);
	}
	else if (size < 65536) {
		rc = as_pack_int16(pk, 0xdc, (uint16_t)size, true);
	} else {
		rc = as_pack_int32(pk, 0xdd, size, true);
	}
				
	if (rc == 0) {
		rc = as_list_foreach(l, as_pack_list_foreach, pk) == true ? 0 : 1;
	}
	return rc;
}

static bool as_pack_map_foreach(const as_val *key, const as_val *val, void *udata)
{
	as_packer *pk = (as_packer *)udata;
	int rc = as_pack_val(pk, (as_val *)key);
	
	if (rc == 0) {
		rc = as_pack_val(pk, (as_val *)val);
	}
	return rc == 0;
}

static int as_pack_map(as_packer *pk, as_map *m)
{
	uint32_t size = as_map_size(m);
	int rc;
	
	if (size < 16) {
		rc = as_pack_byte(pk, (uint8_t)(0x80 | size), true);
	} else if (size < 65536) {
		rc = as_pack_int16(pk, 0xde, (uint16_t)size, true);
	} else {
		rc = as_pack_int32(pk, 0xdf, size, true);
	}
	
	if (rc == 0) {
		rc = as_map_foreach(m, as_pack_map_foreach, pk) == true ? 0 : 1;
	}
	return rc;
}

static int as_pack_rec(as_packer *pk, as_rec *r)
{
	return 1;
}

static int as_pack_pair(as_packer *pk, as_pair *p)
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

int as_pack_val(as_packer *pk, const as_val *val)
{
	int rc = 0;

	if (val == NULL) {
		rc = 1;
	}
	else {	
		switch (as_val_type(val)) {
			case AS_NIL : 
				rc = as_pack_byte(pk, 0xc0, true);
				break;
			case AS_BOOLEAN : 
				rc = as_pack_boolean(pk, (as_boolean *)val);
				break;
			case AS_INTEGER : 
				rc = as_pack_integer(pk, (as_integer *)val);
				break;
			case AS_DOUBLE :
				rc = as_pack_double(pk, (as_double *)val);
				break;
			case AS_STRING :
				rc = as_pack_string(pk, (as_string *)val);
				break;
			case AS_BYTES : 
				rc = as_pack_bytes(pk, (as_bytes *)val);
				break;
			case AS_LIST : 
				rc = as_pack_list(pk, (as_list *)val);
				break;
			case AS_MAP : 
				rc = as_pack_map(pk, (as_map *)val);
				break;
			case AS_REC : 
				rc = as_pack_rec(pk, (as_rec *)val);
				break;
			case AS_PAIR : 
				rc = as_pack_pair(pk, (as_pair *)val);
				break;
			case AS_GEOJSON : 
				rc = as_pack_geojson(pk, (as_geojson *)val);
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

static inline uint16_t as_extract_uint16(as_unpacker *pk)
{
	uint16_t v = *(uint16_t *)(pk->buffer + pk->offset);
	uint16_t swapped = cf_swap_from_be16(v);
	pk->offset += 2;
	return swapped;
}

static inline uint32_t as_extract_uint32(as_unpacker *pk)
{
	uint32_t v = *(uint32_t *)(pk->buffer + pk->offset);
	uint32_t swapped = cf_swap_from_be32(v);
	pk->offset += 4;
	return swapped;
}

static inline uint64_t as_extract_uint64(as_unpacker *pk)
{
	uint64_t v = *(uint64_t *)(pk->buffer + pk->offset);
	uint64_t swapped = cf_swap_from_be64(v);
	pk->offset += 8;
	return swapped;
}

static inline float as_extract_float(as_unpacker *pk)
{
	uint32_t v = *(uint32_t *)(pk->buffer + pk->offset);
	uint32_t swapped = cf_swap_from_be32(v);
	pk->offset += 4;
	return *(float*)&swapped;
}

static inline double as_extract_double(as_unpacker *pk)
{
	uint64_t v = *(uint64_t *)(pk->buffer + pk->offset);
	uint64_t swapped = cf_swap_from_be64(v);
	pk->offset += 8;
	return *(double*)&swapped;
}

static inline int as_unpack_nil(as_val **v)
{
	*v = (as_val *)&as_nil;
	return 0;
}

static inline int as_unpack_boolean(bool b, as_val **v)
{
	// Aerospike does not support boolean, so we convert it to integer.
	*v = (as_val *)as_integer_new(b == true ? 1 : 0);
	return 0;
}

static inline int as_unpack_integer_val(int64_t i, as_val **v)
{
	*v = (as_val *)as_integer_new(i);
	return 0;
}

static inline int as_unpack_double_val(double d, as_val **v)
{
	*v = (as_val *)as_double_new(d);
	return 0;
}

static int as_unpack_blob(as_unpacker *pk, int size, as_val **val)
{
	unsigned char type = pk->buffer[pk->offset++];
	size--;
	
	if (type == AS_BYTES_STRING) {
		char* v = cf_strndup((const char *)pk->buffer + pk->offset, size);
		*val = (as_val*) as_string_new(v, true);
	}
	else if (type == AS_BYTES_GEOJSON) {
		char* v = cf_strndup((const char *)pk->buffer + pk->offset, size);
		*val = (as_val *)as_geojson_new(v, true);
	}
	else {
		unsigned char *buf = cf_malloc(size);
		memcpy(buf, pk->buffer + pk->offset, size);
		as_bytes *b = as_bytes_new_wrap(buf, size, true);
		if (b) {
			b->type = (as_bytes_type) type;
		}
		*val = (as_val *)b;
	}
	pk->offset += size;
	return 0;
}

static int as_unpack_list(as_unpacker *pk, int size, as_val **val)
{
	as_arraylist *list = as_arraylist_new(size, 8);
	
	for (int i = 0; i < size; i++) {
		as_val *v = 0;
		as_unpack_val(pk, &v);
		
		if (v) {
			as_arraylist_set(list, i, v);
		}
	}
	*val = (as_val *)list;
	return 0;
}

static int as_unpack_map_create_list(as_unpacker *pk, int size, as_val **val)
{
	// Create list of key value pairs.
	as_arraylist *list = as_arraylist_new(size, size);

	for (int i = 0; i < size; i++) {
		as_val *k = 0;
		as_val *v = 0;
		if (as_unpack_val(pk, &k) != 0) {
			as_arraylist_destroy(list);
			return -1;
		}
		if (as_unpack_val(pk, &v) != 0) {
			as_val_destroy(k);
			as_arraylist_destroy(list);
			return -2;
		}

		if (k && v) {
			as_pair *pair = as_pair_new(k, v);
			as_arraylist_append(list, (as_val *)pair);
		}
		else {
			as_val_destroy(k);
			as_val_destroy(v);
		}
	}
	*val = (as_val *)list;
	return 0;
}

static int as_unpack_map(as_unpacker *pk, int size, as_val **val)
{
	uint8_t flags = 0;

	// Skip ext element key which is only at the start for metadata.
	if (as_unpack_peek_is_ext(pk)) {
		as_msgpack_ext ext;

		as_unpack_ext(pk, &ext);

		if (as_unpack_size(pk) < 0) {
			return -1;
		}

		flags = ext.type;
		size--;
	}

	// Check preserve order bit.
	if (flags & AS_PACKED_MAP_FLAG_PRESERVE_ORDER) {
		return as_unpack_map_create_list(pk, size, val);
	}

	as_hashmap *map = as_hashmap_new(size > 32 ? size : 32);

	for (int i = 0; i < size; i++) {
		as_val *k = 0;
		as_val *v = 0;
		if (as_unpack_val(pk, &k) != 0) {
			as_hashmap_destroy(map);
			return -3;
		}
		if (as_unpack_val(pk, &v) != 0) {
			as_val_destroy(k);
			as_hashmap_destroy(map);
			return -4;
		}

		if (k && v) {
			as_hashmap_set(map, k, v);
 		}
		else {
			as_val_destroy(k);
			as_val_destroy(v);
		}
	}
	*val = (as_val *)map;
	map->_.flags = flags;
	return 0;
}

int as_unpack_val(as_unpacker *pk, as_val **val)
{
	if (as_unpack_peek_is_ext(pk)) {
		as_unpack_size(pk);
		*val = NULL;
		return 0;
	}

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
		rc = as_pack_byte(pk, (uint8_t)(0x90 | ele_count), false);
	}
	else if (ele_count < 65536) {
		rc = as_pack_int16(pk, 0xdc, (uint16_t)ele_count, false);
	}
	else {
		rc = as_pack_int32(pk, 0xdd, ele_count, false);
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

int as_pack_map_header(as_packer *pk, uint32_t ele_count)
{
	int rc;
	if (ele_count < 16) {
		rc = as_pack_byte(pk, (uint8_t)(0x80 | ele_count), false);
	}
	else if (ele_count < 65536) {
		rc = as_pack_int16(pk, 0xde, (uint16_t)ele_count, false);
	}
	else {
		rc = as_pack_int32(pk, 0xdf, ele_count, false);
	}
	return rc;
}

uint32_t as_pack_ext_header_get_size(uint32_t content_size)
{
	if (content_size < 256) {
		return 1 + 1 + 1;
	}
	else if (content_size < 0x10000) {
		return 1 + 2 + 1;
	}

	return 1 + 4 + 1;
}

int as_pack_ext_header(as_packer *pk, uint32_t content_size, uint8_t type)
{
	int rc;
	if (content_size < 256) {
		rc = as_pack_int8(pk, 0xc7, (uint8_t)content_size, false);
	}
	else if (content_size < 0x10000) {
		rc = as_pack_int16(pk, 0xc8, (uint16_t)content_size, false);
	}
	else {
		rc = as_pack_int32(pk, 0xc9, content_size, false);
	}

	if (rc != 0) {
		return rc;
	}

	return as_pack_byte(pk, type, false);
}

int as_pack_buf_ext_header(uint8_t *buf, uint32_t size, uint32_t content_size, uint8_t type)
{
	as_packer pk = {
			.head = NULL,
			.tail = NULL,
			.buffer = buf,
			.offset = 0,
			.capacity = size
	};

	return as_pack_ext_header(&pk, content_size, type);
}

/******************************************************************************
 * Unpack direct functions
 ******************************************************************************/

/**
 * Get size of list with ele_count elements.
 * Assume header already extracted.
 * @return -1 on failure
 */
static int64_t as_unpack_list_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		state->index = 0;
		state->map_pair = 0;
		state->len = ele_count;
		state->type = AS_LIST;

		int64_t ret = as_unpack_size_non_recursive(pk, block, state);
		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t total = 0;
	for (uint32_t i = 0; i < ele_count; i++) {
		int64_t ret = as_unpack_size_internal(pk, depth);
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
 * @return negative on failure
 */
static int64_t as_unpack_map_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		state->index = 0;
		state->map_pair = 0;
		state->len = ele_count;
		state->type = AS_MAP;

		int64_t ret = as_unpack_size_non_recursive(pk, block, state);
		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t total = 0;
	for (uint32_t i = 0; i < ele_count; i++) {
		int64_t ret = as_unpack_size_internal(pk, depth);
		if (ret < 0) {
			return -1;
		}
		total += ret;

		ret = as_unpack_size_internal(pk, depth);
		if (ret < 0) {
			return -2;
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
	if (pk->length <= pk->offset) {
		return AS_UNDEF;
	}

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

static int64_t as_unpack_size_non_recursive(as_unpacker *pk, msgpack_parse_memblock *block, msgpack_parse_state *state)
{
	int start = pk->offset;

	while (state) {
		while (state->index >= state->len) {
			if (! msgpack_parse_memblock_has_prev(block)) {
				return (int64_t)(pk->offset - start);
			}

			state = msgpack_parse_memblock_prev(&block);
		}

		if (state->type == AS_LIST) {
			state->index++;
		}
		else if (state->type == AS_MAP) {
			if (state->map_pair == 0) {
				state->map_pair++;
			}
			else {
				state->map_pair = 0;
				state->index++;
			}
		}
		else {
			return -1;
		}

		as_val_t type = as_unpack_peek_type(pk);

		if (type == AS_UNDEF) {
			return -2;
		}

		if (type == AS_LIST ) {
			state = msgpack_parse_memblock_next(&block);

			if (! msgpack_parse_state_list_size_init(state, pk)) {
				return -3;
			}

			continue;
		}

		if (type == AS_MAP) {
			state = msgpack_parse_memblock_next(&block);

			if (! msgpack_parse_state_map_size_init(state, pk)) {
				return -4;
			}

			continue;
		}

		if (as_unpack_size_internal(pk, 0) < 0) {
			return -5;
		}
	}

	return -6;
}

static inline int64_t as_unpack_size_internal(as_unpacker *pk, uint32_t depth)
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
			int64_t ret = as_unpack_list_elements_size(pk, length, depth);
			if (ret < 0) {
				return -2;
			}
			return 1 + 2 + ret;
		}

		case 0xdd: { // list with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			int64_t ret = as_unpack_list_elements_size(pk, length, depth);
			if (ret < 0) {
				return -3;
			}
			return 1 + 4 + ret;
		}

		case 0xde: { // map with 16 bit header
			uint16_t length = as_extract_uint16(pk);
			int64_t ret = as_unpack_map_elements_size(pk, length, depth);
			if (ret < 0) {
				return -4;
			}
			return 1 + 2 + ret;
		}

		case 0xdf: { // map with 32 bit header
			uint32_t length = as_extract_uint32(pk);
			int64_t ret = as_unpack_map_elements_size(pk, length, depth);
			if (ret < 0) {
				return -5;
			}
			return 1 + 4 + ret;
		}

		case 0xd4:	// fixext 1
			pk->offset += 1 + 1;
			return 1 + 1 + 1;
		case 0xd5:	// fixext 2
			pk->offset += 1 + 2;
			return 1 + 1 + 2;
		case 0xd6:	// fixext 4
			pk->offset += 1 + 4;
			return 1 + 1 + 4;
		case 0xd7:	// fixext 8
			pk->offset += 1 + 8;
			return 1 + 1 + 8;
		case 0xd8:	// fixext 16
			pk->offset += 1 + 16;
			return 1 + 1 + 16;
		case 0xc7: {// ext 8
			uint8_t length = pk->buffer[pk->offset++];
			pk->offset += 1 + length;
			return 1 + 1 + 1 + length;
		}
		case 0xc8: {// ext 16
			uint16_t length = as_extract_uint16(pk);
			pk->offset += 1 + length;
			return 1 + 2 + 1 + length;
		}
		case 0xc9: {// ext 32
			uint32_t length = as_extract_uint32(pk);
			pk->offset += 1 + length;
			return 1 + 4 + 1 + length;
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
		int64_t ret = as_unpack_map_elements_size(pk, type & 0x0f, depth);
		if (ret < 0) {
			return -6;
		}
		return 1 + ret;
	}

	if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
		int64_t ret = as_unpack_list_elements_size(pk, type & 0x0f, depth);
		if (ret < 0) {
			return -7;
		}
		return 1 + ret;
	}

	if (type < 0x80) { // 8 bit combined unsigned integer
		return 1;
	}

	if (type >= 0xe0) { // 8 bit combined signed integer
		return 1;
	}

	return -8;
}

int64_t as_unpack_size(as_unpacker *pk)
{
	return as_unpack_size_internal(pk, 0);
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

int as_unpack_ext(as_unpacker *pk, as_msgpack_ext *ext)
{
	// Need at least 3 bytes.
	if (pk->length - pk->offset < 3) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];

	switch (type) {
	case 0xd4:	// fixext 1
		ext->size = 1;
		break;
	case 0xd5:	// fixext 2
		ext->size = 2;
		break;
	case 0xd6:	// fixext 4
		ext->size = 4;
		break;
	case 0xd7:	// fixext 8
		ext->size = 8;
		break;
	case 0xd8:	// fixext 16
		ext->size = 16;
		break;
	case 0xc7:	// ext 8
		ext->size = (uint32_t)pk->buffer[pk->offset++];
		break;
	case 0xc8:	// ext 16
		ext->size = as_extract_uint16(pk);
		break;
	case 0xc9: {// ext 32
		// Need at least 4 more bytes.
		if (pk->length - pk->offset < 4) {
			return -2;
		}
		ext->size = as_extract_uint32(pk);
		break;
	}
	default:
		return -3;
	}

	if (pk->length - pk->offset < 1 + ext->size) {
		return -4;
	}

	ext->type_offset = pk->offset;
	ext->type = pk->buffer[pk->offset++];
	ext->data = pk->buffer + pk->offset;
	pk->offset += ext->size;

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

/******************************************************************************
 * Compare direct functions
 ******************************************************************************/

int64_t as_unpack_map_header_element_count(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
		case 0xde: { // map with 16 bit header
			if (size < 2) {
				return -2;
			}
			return as_extract_uint16(pk);
		}
		case 0xdf: { // map with 32 bit header
			if (size < 4) {
				return -3;
			}
			return as_extract_uint32(pk);
		}
		default:
			break;
	}

	if ((type & 0xf0) == 0x80) { // map with 8 bit combined header
		return type & 0x0f;
	}

	return -4;
}

int64_t as_unpack_buf_map_element_count(const uint8_t *buf, uint32_t size)
{
	as_unpacker pk = {
			.buffer = buf,
			.offset = 0,
			.length = size,
	};

	return as_unpack_map_header_element_count(&pk);
}

bool as_unpack_peek_is_ext(const as_unpacker *pk)
{
	uint8_t type = pk->buffer[pk->offset];

	switch (type) {
	case 0xd4:
	case 0xd5:
	case 0xd6:
	case 0xd7:
	case 0xd8:
	case 0xc7:
	case 0xc8:
	case 0xc9:
		return true;
	default:
		break;
	}

	return false;
}

static inline msgpack_compare_t msgpack_compare_int(as_unpacker *pk1, as_unpacker *pk2)
{
	int64_t v1 = 0;
	int64_t v2 = 0;

	if (as_unpack_int64(pk1, &v1) != 0
			|| as_unpack_int64(pk2, &v2) != 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(v1, v2);

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t msgpack_compare_double(as_unpacker *pk1, as_unpacker *pk2)
{
	double v1 = 0;
	double v2 = 0;

	if (as_unpack_double(pk1, &v1) != 0
			|| as_unpack_double(pk2, &v2) != 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(v1, v2);

	return MSGPACK_COMPARE_EQUAL;
}

static inline int64_t msgpack_get_blob_len(as_unpacker *pk)
{
	int64_t len = as_unpack_blob_size(pk);

	if (len == 0) {
		return -1;
	}

	// Adjust for 1 byte type.
	len--;
	pk->offset++;

	int64_t left = pk->length - pk->offset;

	if (len > left) {
		return left;
	}

	return len;
}

static msgpack_compare_t msgpack_compare_blob_internal(as_unpacker *pk1, int64_t len1, as_unpacker *pk2, int64_t len2)
{
	int64_t minlen = (len1 < len2) ? len1 : len2;

	if (minlen < 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	int offset1 = pk1->offset;
	int offset2 = pk2->offset;

	pk1->offset += len1;
	pk2->offset += len2;

	for (int64_t i = 0; i < minlen; i++) {
		unsigned char c1 = pk1->buffer[offset1++];
		unsigned char c2 = pk2->buffer[offset2++];

		MSGPACK_COMPARE_RET_LESS_OR_GREATER(c1, c2);
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(len1, len2);

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t msgpack_compare_blob(as_unpacker *pk1, as_unpacker *pk2)
{
	int64_t len1 = msgpack_get_blob_len(pk1);
	int64_t len2 = msgpack_get_blob_len(pk2);

	return msgpack_compare_blob_internal(pk1, len1, pk2, len2);
}

static inline msgpack_compare_t msgpack_compare_int64_t(int64_t x1, int64_t x2)
{
	MSGPACK_COMPARE_RET_LESS_OR_GREATER(x1, x2);

	return MSGPACK_COMPARE_EQUAL;
}

// Skip n vals.
static bool msgpack_skip(as_unpacker *pk, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (as_unpack_size(pk) < 0) {
			return false;
		}
	}
	return true;
}

static bool msgpack_compare_unwind(as_unpacker *pk1, as_unpacker *pk2, msgpack_parse_memblock *block, msgpack_parse_state *state)
{
	while (true) {
		if (state->type == AS_LIST) {
			if (! msgpack_skip(pk1, state->len1 - state->index)) {
				return false;
			}
			if (! msgpack_skip(pk2, state->len2 - state->index)) {
				return false;
			}
		}
		else if (state->type == AS_MAP) {
			if (! msgpack_skip(pk1, 2 * (state->len1 - state->index) - state->map_pair)) {
				return false;
			}
			if (! msgpack_skip(pk2, 2 * (state->len2 - state->index) - state->map_pair)) {
				return false;
			}
		}

		if (! msgpack_parse_memblock_has_prev(block)) {
			break;
		}

		state = msgpack_parse_memblock_prev(&block);
	}
	return true;
}

static msgpack_compare_t msgpack_compare_list(as_unpacker *pk1, as_unpacker *pk2, size_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		if (! msgpack_parse_state_list_cmp_init(state, pk1, pk2)) {
			return MSGPACK_COMPARE_ERROR;
		}

		msgpack_compare_t ret = msgpack_compare_non_recursive(pk1, pk2, block, state);

		if (! msgpack_compare_unwind(pk1, pk2, block, state)) {
			msgpack_parse_memblock_destroy(block);
			return MSGPACK_COMPARE_ERROR;
		}

		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t len1 = as_unpack_list_header_element_count(pk1);
	int64_t len2 = as_unpack_list_header_element_count(pk2);
	int64_t minlen = (len1 < len2) ? len1 : len2;

	if (minlen < 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	for (int64_t i = 0; i < minlen; i++) {
		msgpack_compare_t ret = msgpack_compare_internal(pk1, pk2, depth);

		if (ret != MSGPACK_COMPARE_EQUAL) {
			if (! msgpack_skip(pk1, len1 - i - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}
			if (! msgpack_skip(pk2, len2 - i - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}
			return ret;
		}
	}

	return MSGPACK_COMPARE_EQUAL;
}

static msgpack_compare_t msgpack_compare_map(as_unpacker *pk1, as_unpacker *pk2, size_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		if (! msgpack_parse_state_map_cmp_init(state, pk1, pk2)) {
			return MSGPACK_COMPARE_ERROR;
		}

		if (state->default_compare_type != MSGPACK_COMPARE_EQUAL) {
			msgpack_parse_memblock_destroy(block);
			return state->default_compare_type;
		}

		msgpack_compare_t ret = msgpack_compare_non_recursive(pk1, pk2, block, state);
		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t len1 = as_unpack_map_header_element_count(pk1);
	int64_t len2 = as_unpack_map_header_element_count(pk2);
	int64_t minlen = (len1 < len2) ? len1 : len2;

	if (minlen < 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(len1, len2);

	for (int64_t i = 0; i < minlen; i++) {
		msgpack_compare_t ret = msgpack_compare_internal(pk1, pk2, depth);

		if (ret != MSGPACK_COMPARE_EQUAL) {
			if (! msgpack_skip(pk1, 2 * (len1 - i) - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}
			if (! msgpack_skip(pk2, 2 * (len2 - i) - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}
			return ret;
		}

		ret = msgpack_compare_internal(pk1, pk2, depth);

		if (ret != MSGPACK_COMPARE_EQUAL) {
			if (! msgpack_skip(pk1, 2 * (len1 - i - 1))) {
				return MSGPACK_COMPARE_ERROR;
			}
			if (! msgpack_skip(pk2, 2 * (len2 - i - 1))) {
				return MSGPACK_COMPARE_ERROR;
			}
			return ret;
		}
	}

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t msgpack_peek_compare_type(const as_unpacker *pk1, const as_unpacker *pk2, as_val_t *type)
{
	int n1 = pk1->length - pk1->offset;
	int n2 = pk2->length - pk2->offset;

	if (n1 == 0 || n2 == 0) {
		MSGPACK_COMPARE_RET_LESS_OR_GREATER(n1, n2);

		return MSGPACK_COMPARE_END;
	}

	as_val_t type1 = as_unpack_peek_type(pk1);
	as_val_t type2 = as_unpack_peek_type(pk2);

	if (type1 == AS_UNDEF || type2 == AS_UNDEF) {
		return MSGPACK_COMPARE_ERROR;
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(type1, type2);

	*type = type1;

	return MSGPACK_COMPARE_EQUAL;
}

static msgpack_compare_t msgpack_compare_non_recursive(as_unpacker *pk1, as_unpacker *pk2, msgpack_parse_memblock *block, msgpack_parse_state *state)
{
	while (state) {
		while (state->index >= state->len) {
			if (! msgpack_parse_memblock_has_prev(block)) {
				return state->default_compare_type;
			}

			state = msgpack_parse_memblock_prev(&block);
		}

		if (state->type == AS_LIST) {
			state->index++;
		}
		else if (state->type == AS_MAP) {
			if (state->map_pair == 0) {
				state->map_pair++;
			}
			else {
				state->map_pair = 0;
				state->index++;
			}
		}
		else {
			return MSGPACK_COMPARE_ERROR;
		}

		as_val_t type;
		msgpack_compare_t ret = msgpack_peek_compare_type(pk1, pk2, &type);

		if (ret == MSGPACK_COMPARE_END) {
			return MSGPACK_COMPARE_EQUAL;
		}

		if (ret != MSGPACK_COMPARE_EQUAL) {
			if (as_unpack_size(pk1) < 0) {
				return MSGPACK_COMPARE_ERROR;
			}
			if (as_unpack_size(pk2) < 0) {
				return MSGPACK_COMPARE_ERROR;
			}
			return ret;
		}

		if (type == AS_LIST ) {
			state = msgpack_parse_memblock_next(&block);

			if (! msgpack_parse_state_list_cmp_init(state, pk1, pk2)) {
				return MSGPACK_COMPARE_ERROR;
			}

			continue;
		}

		if (type == AS_MAP) {
			state = msgpack_parse_memblock_next(&block);

			if (! msgpack_parse_state_map_cmp_init(state, pk1, pk2)) {
				return MSGPACK_COMPARE_ERROR;
			}

			continue;
		}

		ret = msgpack_compare_internal(pk1, pk2, 0);

		if (ret != MSGPACK_COMPARE_EQUAL) {
			return ret;
		}
	}

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t msgpack_compare_type(as_unpacker *pk1, as_unpacker *pk2, as_val_t type, size_t depth)
{
	switch (type) {
	case AS_NIL:
		pk1->offset++;
		pk2->offset++;

		break;
	case AS_BOOLEAN: {
		unsigned char byte1 = pk1->buffer[pk1->offset++];
		unsigned char byte2 = pk2->buffer[pk2->offset++];

		if (byte1 < byte2) {
			return MSGPACK_COMPARE_LESS;
		}

		if (byte1 > byte2) {
			return MSGPACK_COMPARE_GREATER;
		}

		break;
	}
	case AS_INTEGER:
		return msgpack_compare_int(pk1, pk2);
	case AS_STRING:
		return msgpack_compare_blob(pk1, pk2);
	case AS_LIST:
		return msgpack_compare_list(pk1, pk2, depth);
	case AS_MAP:
		return msgpack_compare_map(pk1, pk2, depth);
	case AS_BYTES:
		return msgpack_compare_blob(pk1, pk2);
	case AS_DOUBLE:
		return msgpack_compare_double(pk1, pk2);
	case AS_GEOJSON:
		return msgpack_compare_blob(pk1, pk2);
	default:
		return MSGPACK_COMPARE_ERROR;
	}

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t msgpack_compare_internal(as_unpacker *pk1, as_unpacker *pk2, size_t depth)
{
	as_val_t type;
	msgpack_compare_t ret = msgpack_peek_compare_type(pk1, pk2, &type);

	if (ret != MSGPACK_COMPARE_EQUAL) {
		if (as_unpack_size(pk1) < 0) {
			return MSGPACK_COMPARE_ERROR;
		}
		if (as_unpack_size(pk2) < 0) {
			return MSGPACK_COMPARE_ERROR;
		}
		return ret;
	}

	return msgpack_compare_type(pk1, pk2, type, depth);
}

msgpack_compare_t as_unpack_buf_compare(const uint8_t *buf1, uint32_t size1, const uint8_t *buf2, uint32_t size2)
{
	as_unpacker pk1 = {
		.buffer = buf1,
		.offset = 0,
		.length = size1
	};
	as_unpacker pk2 = {
		.buffer = buf2,
		.offset = 0,
		.length = size2
	};

	return msgpack_compare_internal(&pk1, &pk2, 0);
}

msgpack_compare_t as_unpack_compare(as_unpacker *pk1, as_unpacker *pk2)
{
	return msgpack_compare_internal(pk1, pk2, 0);
}
