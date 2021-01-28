/* 
 * Copyright 2008-2020 Aerospike, Inc.
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

#include <string.h>

#include <aerospike/as_msgpack_ext.h>
#include <aerospike/as_serializer.h>
#include <aerospike/as_types.h>
#include <aerospike/as_vector.h>

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_byte_order.h>

/******************************************************************************
 * INTERNAL TYPEDEFS & CONSTANTS
 ******************************************************************************/

#define MSGPACK_COMPARE_MAX_DEPTH	256
#define MSGPACK_PARSE_MEMBLOCK_STATE_COUNT	256

#define ASVAL_CMP_EXT_TYPE	0xFF
#define ASVAL_CMP_WILDCARD	0x00
#define ASVAL_CMP_INF		0x01

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
static inline bool msgpack_parse_memblock_has_prev(const msgpack_parse_memblock *block);
static msgpack_parse_state *msgpack_parse_memblock_prev(msgpack_parse_memblock **block);
static bool msgpack_parse_state_list_cmp_init(msgpack_parse_state *state, as_unpacker *pk1, as_unpacker *pk2);
static bool msgpack_parse_state_list_size_init(msgpack_parse_state *state, as_unpacker *pk);
static bool msgpack_parse_state_map_cmp_init(msgpack_parse_state *state, as_unpacker *pk1, as_unpacker *pk2);
static bool msgpack_parse_state_map_size_init(msgpack_parse_state *state, as_unpacker *pk);

// msgpack_compare
static inline msgpack_compare_t msgpack_compare_int(as_unpacker *pk1, as_unpacker *pk2);
static inline msgpack_compare_t msgpack_compare_double(as_unpacker *pk1, as_unpacker *pk2);
static inline int64_t msgpack_get_blob_len(as_unpacker *pk);
static msgpack_compare_t msgpack_compare_blob_internal(as_unpacker *pk1, uint32_t len1, as_unpacker *pk2, uint32_t len2);
static inline msgpack_compare_t msgpack_compare_blob(as_unpacker *pk1, as_unpacker *pk2);
static inline msgpack_compare_t msgpack_compare_int64_t(int64_t x1, int64_t x2);
static bool msgpack_skip(as_unpacker *pk, size_t n);
static bool msgpack_compare_unwind(as_unpacker *pk1, as_unpacker *pk2, const msgpack_parse_state *state);
static bool msgpack_compare_unwind_all(as_unpacker *pk1, as_unpacker *pk2, msgpack_parse_memblock **block);
static msgpack_compare_t msgpack_compare_list(as_unpacker *pk1, as_unpacker *pk2, size_t depth);
static msgpack_compare_t msgpack_compare_map(as_unpacker *pk1, as_unpacker *pk2, size_t depth);
static inline msgpack_compare_t msgpack_peek_compare_type(const as_unpacker *pk1, const as_unpacker *pk2, as_val_t *type);
static msgpack_compare_t msgpack_compare_non_recursive(as_unpacker *pk1, as_unpacker *pk2, msgpack_parse_memblock **block, msgpack_parse_state *state);
static inline msgpack_compare_t msgpack_compare_type(as_unpacker *pk1, as_unpacker *pk2, as_val_t type, size_t depth);
static inline msgpack_compare_t msgpack_compare_internal(as_unpacker *pk1, as_unpacker *pk2, size_t depth, as_val_t *type);

// pack direct
static int as_pack_inf_internal(as_packer *pk, bool resize);
static int as_pack_wildcard_internal(as_packer *pk, bool resize);

// unpack direct
static int64_t unpack_list_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth);
static int64_t unpack_map_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth);
static inline as_val_t bytes_internal_type_to_as_val_t(uint8_t type);
static int64_t unpack_size_non_recursive(as_unpacker *pk, msgpack_parse_memblock *block, msgpack_parse_state *state);
static inline int64_t unpack_size_internal(as_unpacker *pk, uint32_t depth);
static inline const uint8_t *unpack_str_bin(as_unpacker *pk, uint32_t *sz_r);


/******************************************************************************
 * MSGPACK_PARSE FUNCTIONS
 ******************************************************************************/

static msgpack_parse_memblock *
msgpack_parse_memblock_create(msgpack_parse_memblock *prev)
{
	msgpack_parse_memblock *p = cf_malloc(sizeof(msgpack_parse_memblock));
	p->prev = prev;
	p->count = 0;
	return p;
}

static void
msgpack_parse_memblock_destroy(msgpack_parse_memblock *block)
{
	while (block) {
		msgpack_parse_memblock *p = block;
		block = block->prev;
		cf_free(p);
	}
}

static msgpack_parse_state *
msgpack_parse_memblock_next(msgpack_parse_memblock **block)
{
	msgpack_parse_memblock *ptr = *block;

	if (ptr->count >= MSGPACK_PARSE_MEMBLOCK_STATE_COUNT) {
		ptr = msgpack_parse_memblock_create(ptr);
		*block = ptr;
	}

	return &ptr->buffer[ptr->count++];
}

static inline bool
msgpack_parse_memblock_has_prev(const msgpack_parse_memblock *block)
{
	if (block->prev || block->count > 1) {
		return true;
	}

	return false;
}

static msgpack_parse_state *
msgpack_parse_memblock_prev(msgpack_parse_memblock **block)
{
	msgpack_parse_memblock *ptr = *block;

	if (ptr->count <= 1) {
		ptr = ptr->prev;
		cf_free(*block);
		*block = ptr;
	}
	else {
		ptr->count--;
	}

	// No check for NULL ptr here, use has_prev to make sure it doesn't happen.

	return &ptr->buffer[ptr->count - 1];
}

static bool
msgpack_parse_state_list_cmp_init(msgpack_parse_state *state, as_unpacker *pk1,
		as_unpacker *pk2)
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

static bool
msgpack_parse_state_list_size_init(msgpack_parse_state *state, as_unpacker *pk)
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

static bool
msgpack_parse_state_map_cmp_init(msgpack_parse_state *state, as_unpacker *pk1,
		as_unpacker *pk2)
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

static bool
msgpack_parse_state_map_size_init(msgpack_parse_state *state, as_unpacker *pk)
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

static int
pack_resize(as_packer *pk, uint32_t sz)
{
	// Add current buffer to linked list and allocate a new buffer
	as_packer_buffer *entry =
			(as_packer_buffer *)cf_malloc(sizeof(as_packer_buffer));

	if (! entry) {
		return -1;
	}

	entry->buffer = pk->buffer;
	entry->length = pk->offset;
	entry->next = 0;

	size_t newcap = (size_t)((sz > (uint32_t)pk->capacity) ? sz : pk->capacity);

	if (! (pk->buffer = (unsigned char *)cf_malloc(newcap))) {
		cf_free(entry);
		return -1;
	}

	pk->capacity = (int)newcap;
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

static inline int
pack_append(as_packer *pk, const unsigned char *src, uint32_t sz, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + sz > pk->capacity) {
			if (! resize || pack_resize(pk, sz) != 0) {
				return -1;
			}
		}
		memcpy(pk->buffer + pk->offset, src, (size_t)sz);
	}
	pk->offset += sz;
	return 0;
}

static inline int
pack_byte(as_packer *pk, uint8_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 1 > pk->capacity) {
			if (! resize || pack_resize(pk, 1) != 0) {
				return -1;
			}
		}
		*(pk->buffer + pk->offset) = val;
	}
	pk->offset++;
	return 0;
}

static inline int
pack_type_uint8(as_packer *pk, unsigned char type, uint8_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 2 > pk->capacity) {
			if (! resize || pack_resize(pk, 2) != 0) {
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

static inline int
pack_type_uint16(as_packer *pk, unsigned char type, uint16_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 3 > pk->capacity) {
			if (! resize || pack_resize(pk, 3) != 0) {
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

static inline int
pack_type_uint32(as_packer *pk, unsigned char type, uint32_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 5 > pk->capacity) {
			if (! resize || pack_resize(pk, 5) != 0) {
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

static inline int
pack_type_uint64(as_packer *pk, unsigned char type, uint64_t val, bool resize)
{
	if (pk->buffer) {
		if (pk->offset + 9 > pk->capacity) {
			if (! resize || pack_resize(pk, 9) != 0) {
				return -1;
			}
		}
		uint64_t swapped = cf_swap_to_be64(val);
		unsigned char *p = pk->buffer + pk->offset;
		*p++ = type;
		memcpy(p, &swapped, 8);
	}
	pk->offset += 9;
	return 0;
}

static inline int
pack_as_boolean(as_packer *pk, const as_boolean *b)
{
	return pack_byte(pk, as_boolean_get(b) ? 0xc3 : 0xc2, true);
}

static inline int
pack_uint64(as_packer *pk, uint64_t val, bool resize)
{
	if (val < (1ULL << 7)) {
		return pack_byte(pk, (uint8_t)val, resize);
	}

	if (val < (1ULL << 8)) {
		return pack_type_uint8(pk, 0xcc, (uint8_t)val, resize);
	}

	if (val < (1ULL << 16)) {
		return pack_type_uint16(pk, 0xcd, (uint16_t)val, resize);
	}

	if (val < (1ULL << 32)) {
		return pack_type_uint32(pk, 0xce, (uint32_t)val, resize);
	}

	return pack_type_uint64(pk, 0xcf, val, resize);
}

static inline int
pack_int64(as_packer *pk, int64_t val, bool resize)
{
	if (val >= 0) {
		return pack_uint64(pk, (uint64_t)val, resize);
	}

	if (val >= -(1LL << 5)) {
		return pack_byte(pk, (uint8_t)(0xe0 | (val + 32)), resize);
	}

	if (val >= -(1LL << 7)) {
		return pack_type_uint8(pk, 0xd0, (uint8_t)val, resize);
	}

	if (val >= -(1LL << 15)) {
		return pack_type_uint16(pk, 0xd1, (uint16_t)val, resize);
	}

	if (val >= -(1LL << 31)) {
		return pack_type_uint32(pk, 0xd2, (uint32_t)val, resize);
	}

	return pack_type_uint64(pk, 0xd3, (uint64_t)val, resize);
}

static int
pack_as_integer(as_packer *pk, const as_integer *i)
{
	int64_t val = as_integer_get(i);
	return pack_int64(pk, val, true);
}

static inline int
pack_double(as_packer *pk, double val, bool resize)
{
	return pack_type_uint64(pk, 0xcb, *(uint64_t *)&val, resize);
}

static inline int
pack_as_double(as_packer *pk, const as_double *d)
{
	double val = as_double_get(d);
	return pack_double(pk, val, true);
}

static int
pack_string_header(as_packer *pk, uint32_t size)
{
	if (size < 32) {
		return pack_byte(pk, (uint8_t)(0xa0 | size), true);
	}

	if (size < 256) {
		return pack_type_uint8(pk, 0xd9, (uint8_t)size, true);
	}

	if (size < 65536) {
		return pack_type_uint16(pk, 0xda, (uint16_t)size, true);
	}

	return pack_type_uint32(pk, 0xdb, size, true);
}

static inline int
pack_byte_array_header(as_packer *pk, uint32_t size)
{
	// Use string header codes for byte arrays.
	return pack_string_header(pk, size);
}

static int
pack_string(as_packer *pk, as_string *s)
{
	uint32_t length = (uint32_t)as_string_len(s);
	int rc = pack_string_header(pk, length + 1);

	if (rc == 0) {
		rc = pack_byte(pk, AS_BYTES_STRING, true);
	}

	if (rc == 0) {
		rc = pack_append(pk, (unsigned char*)s->value, length, true);
	}

	return rc;
}

static int
pack_geojson(as_packer *pk, as_geojson *s)
{
	uint32_t length = (uint32_t)as_geojson_len(s);
	int rc = pack_byte_array_header(pk, length + 1);

	if (rc == 0) {
		rc = pack_byte(pk, AS_BYTES_GEOJSON, true);
	}

	if (rc == 0) {
		rc = pack_append(pk, (unsigned char*)s->value, length, true);
	}

	return rc;
}

static int
pack_bytes(as_packer *pk, const as_bytes *b)
{
	int rc = pack_byte_array_header(pk, b->size + 1);

	if (rc == 0) {
		rc = pack_byte(pk, b->type, true);
	}

	if (rc == 0) {
		rc = pack_append(pk, b->value, b->size, true);
	}

	return rc;
}

static bool
pack_list_foreach(as_val *val, void *udata)
{
	as_packer *pk = (as_packer *)udata;
	return as_pack_val(pk, val) == 0;
}

static int
pack_list(as_packer *pk, as_list *l)
{
	uint32_t size = as_list_size(l);
	int rc;

	if (size < 16) {
		rc = pack_byte(pk, (uint8_t)(0x90 | size), true);
	}
	else if (size < 65536) {
		rc = pack_type_uint16(pk, 0xdc, (uint16_t)size, true);
	}
	else {
		rc = pack_type_uint32(pk, 0xdd, size, true);
	}

	if (rc == 0) {
		rc = as_list_foreach(l, pack_list_foreach, pk) ? 0 : 1;
	}

	return rc;
}

static bool
pack_map_foreach(const as_val *key, const as_val *val, void *udata)
{
	as_packer *pk = (as_packer *)udata;
	int rc = as_pack_val(pk, (as_val *)key);

	if (rc == 0) {
		rc = as_pack_val(pk, (as_val *)val);
	}

	return rc == 0;
}

static int
as_bytes_cmp(as_bytes* b1, as_bytes* b2)
{
	if (b1->size == b2->size) {
		return memcmp(b1->value, b2->value, b1->size);
	}
	else if (b1->size < b2->size) {
		int cmp = memcmp(b1->value, b2->value, b1->size);
		return cmp != 0 ? cmp : -1;
	}
	else {
		int cmp = memcmp(b1->value, b2->value, b2->size);
		return cmp != 0 ? cmp : 1;
	}
}

static int
as_val_cmp(const as_val* v1, const as_val* v2);

static int
as_list_cmp_max(const as_list* list1, const as_list* list2, uint32_t max, uint32_t fin)
{
	for (uint32_t i = 0; i < max; i++) {
		int cmp = as_val_cmp(as_list_get(list1, i), as_list_get(list2, i));

		if (cmp != 0) {
			return cmp;
		}
	}
	return fin;
}

static int
as_list_cmp(const as_list* list1, const as_list* list2)
{
	uint32_t size1 = as_list_size(list1);
	uint32_t size2 = as_list_size(list2);

	if (size1 == size2) {
		return as_list_cmp_max(list1, list2, size1, 0);
	}
	else if (size1 < size2) {
		return as_list_cmp_max(list1, list2, size1, -1);
	}
	else {
		return as_list_cmp_max(list1, list2, size2, 1);
	}
}

static int
as_vector_cmp(as_vector* list1, as_vector* list2)
{
	// Size of vectors should already be the same.
	for (uint32_t i = 0; i < list1->size; i++) {
		int cmp = as_val_cmp(as_vector_get_ptr(list1, i), as_vector_get_ptr(list2, i));

		if (cmp != 0) {
			return cmp;
		}
	}
	return 0;
}

static bool
key_append(const as_val* key, const as_val* val, void* udata)
{
	as_vector_append(udata, (void*)&key);
	return true;
}

static int
key_cmp(const void* v1, const void* v2)
{
	return as_val_cmp(*(as_val**)v1, *(as_val**)v2);
}

static bool
map_to_sorted_keys(const as_map* map, uint32_t size, as_vector* list)
{
	as_vector_init(list, sizeof(as_val*), size);

	if (! as_map_foreach(map, key_append, list)) {
		return false;
	}

	// Sort list of map entries.
	qsort(list->list, list->size, sizeof(as_val*), key_cmp);
	return true;
}

static int
as_map_cmp(const as_map* map1, const as_map* map2)
{
	// Map ordering documented at https://www.aerospike.com/docs/guide/cdt-ordering.html
	uint32_t size1 = as_map_size(map1);
	uint32_t size2 = as_map_size(map2);
	int cmp = size1 - size2;

	if (cmp != 0) {
		return cmp;
	}

	// Convert maps to lists of keys and sort before comparing.
	as_vector list1;

	if (map_to_sorted_keys(map1, size1, &list1)) {
		as_vector list2;

		if (map_to_sorted_keys(map2, size2, &list2)) {
			cmp = as_vector_cmp(&list1, &list2);
		}
		as_vector_destroy(&list2);
	}
	as_vector_destroy(&list1);
	return cmp;
}

static int
as_val_cmp(const as_val* v1, const as_val* v2)
{
	if (v1->type == AS_CMP_WILDCARD || v2->type == AS_CMP_WILDCARD) {
		return 0;
	}

	if (v1->type != v2->type) {
		return v1->type - v2->type;
	}

	switch (v1->type) {
	case AS_BOOLEAN:
		return ((as_boolean*)v1)->value - ((as_boolean*)v2)->value;

	case AS_INTEGER: {
		int64_t cmp = ((as_integer*)v1)->value - ((as_integer*)v2)->value;
		return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
	}

	case AS_DOUBLE: {
		double cmp = ((as_double*)v1)->value - ((as_double*)v2)->value;
		return cmp < 0.0 ? -1 : cmp > 0.0 ? 1 : 0;
	}

	case AS_STRING:
		return strcmp(((as_string*)v1)->value, ((as_string*)v2)->value);

	case AS_GEOJSON:
		return strcmp(((as_geojson*)v1)->value, ((as_geojson*)v2)->value);

	case AS_BYTES:
		return as_bytes_cmp((as_bytes*)v1, (as_bytes*)v2);

	case AS_LIST:
		return as_list_cmp((as_list*)v1, (as_list*)v2);

	case AS_MAP:
		return as_map_cmp((as_map*)v1, (as_map*)v2);

	default:
		return 0;
	}
}

typedef struct {
	const as_val* key;
	const as_val* val;
} key_val;

static bool
key_val_append(const as_val* key, const as_val* val, void* udata)
{
	key_val kv = {key, val};
	as_vector_append(udata, &kv);
	return true;
}

static int
key_val_cmp(const void* v1, const void* v2)
{
	return as_val_cmp(((key_val*)v1)->key, ((key_val*)v2)->key);
}

static int
pack_map_ordered(as_packer* pk, const as_map* map, uint32_t size)
{
	// Sort map before packing.
	// Copy map to a list.
	as_vector list;

	if (size <= 500) {
		as_vector_inita(&list, sizeof(key_val), size);
	}
	else {
		as_vector_init(&list, sizeof(key_val), size);
	}

	if (!as_map_foreach(map, key_val_append, &list)) {
		as_vector_destroy(&list);
		return 1;
	}

	// Sort list of map entries.
	qsort(list.list, list.size, sizeof(key_val), key_val_cmp);

	// Pack sorted list of map entries.
	for (uint32_t i = 0; i < list.size; i++) {
		key_val* kv = as_vector_get(&list, i);

		if (!pack_map_foreach(kv->key, kv->val, pk)) {
			as_vector_destroy(&list);
			return 1;
		}
	}
	as_vector_destroy(&list);
	return 0;
}

static int
pack_map(as_packer *pk, const as_map *m)
{
	uint32_t size = as_map_size(m);
	int rc;

	if (size < 16) {
		rc = pack_byte(pk, (uint8_t)(0x80 | size), true);
	}
	else if (size < 65536) {
		rc = pack_type_uint16(pk, 0xde, (uint16_t)size, true);
	}
	else {
		rc = pack_type_uint32(pk, 0xdf, size, true);
	}

	if (rc != 0) {
		return rc;
	}

	if (m->flags & AS_PACKED_MAP_FLAG_KV_ORDERED) {
		return pack_map_ordered(pk, m, size);
	}

	// Pack unordered map.
	return as_map_foreach(m, pack_map_foreach, pk) ? 0 : 1;
}

static int
pack_rec(as_packer *pk, const as_rec *r)
{
	return 1;
}

static int
pack_pair(as_packer *pk, as_pair *p)
{
	unsigned char v = (unsigned char)(0x90 | 2);
	int rc = pack_append(pk, &v, 1, true);

	if (rc == 0) {
		rc = as_pack_val(pk, as_pair_1(p));

		if (rc == 0) {
			rc = as_pack_val(pk, as_pair_2(p));
		}
	}

	return rc;
}

int
as_pack_val(as_packer *pk, const as_val *val)
{
	if (! val) {
		return -1;
	}

	int rc = 0;

	// TODO - Make all of these const.
	switch (as_val_type(val)) {
	case AS_NIL:
		rc = pack_byte(pk, 0xc0, true);
		break;
	case AS_BOOLEAN:
		rc = pack_as_boolean(pk, (const as_boolean *)val);
		break;
	case AS_INTEGER:
		rc = pack_as_integer(pk, (const as_integer *)val);
		break;
	case AS_DOUBLE:
		rc = pack_as_double(pk, (const as_double *)val);
		break;
	case AS_STRING:
		rc = pack_string(pk, (as_string *)val);
		break;
	case AS_BYTES:
		rc = pack_bytes(pk, (const as_bytes *)val);
		break;
	case AS_LIST:
		rc = pack_list(pk, (as_list *)val);
		break;
	case AS_MAP:
		rc = pack_map(pk, (const as_map *)val);
		break;
	case AS_REC:
		rc = pack_rec(pk, (const as_rec *)val);
		break;
	case AS_PAIR:
		rc = pack_pair(pk, (as_pair *)val);
		break;
	case AS_GEOJSON:
		rc = pack_geojson(pk, (as_geojson *)val);
		break;
	case AS_CMP_WILDCARD:
		rc = as_pack_wildcard_internal(pk, true);
		break;
	case AS_CMP_INF:
		rc = as_pack_inf_internal(pk, true);
		break;
	default:
		return -2;
	}

	return rc;
}

/******************************************************************************
 * UNPACK FUNCTIONS
 ******************************************************************************/

static inline uint16_t
extract_uint16(as_unpacker *pk)
{
	uint16_t v = *(uint16_t *)(pk->buffer + pk->offset);
	uint16_t swapped = cf_swap_from_be16(v);
	pk->offset += 2;
	return swapped;
}

static inline uint32_t
extract_uint32(as_unpacker *pk)
{
	uint32_t v = *(uint32_t *)(pk->buffer + pk->offset);
	uint32_t swapped = cf_swap_from_be32(v);
	pk->offset += 4;
	return swapped;
}

static inline uint64_t
extract_uint64(as_unpacker *pk)
{
	uint64_t v = *(uint64_t *)(pk->buffer + pk->offset);
	uint64_t swapped = cf_swap_from_be64(v);
	pk->offset += 8;
	return swapped;
}

static inline float
extract_float(as_unpacker *pk)
{
	uint32_t v = *(uint32_t *)(pk->buffer + pk->offset);
	uint32_t swapped = cf_swap_from_be32(v);
	pk->offset += 4;
	return *(float*)&swapped;
}

static inline double
extract_double(as_unpacker *pk)
{
	uint64_t v = *(uint64_t *)(pk->buffer + pk->offset);
	uint64_t swapped = cf_swap_from_be64(v);
	pk->offset += 8;
	return *(double*)&swapped;
}

static inline int
unpack_nil(as_val **v)
{
	*v = (as_val *)&as_nil;
	return 0;
}

static inline int
unpack_ext(as_unpacker *pk, uint8_t type, as_val **v)
{
	uint8_t ext_type = pk->buffer[pk->offset++];
	uint8_t data = pk->buffer[pk->offset++];

	if (ext_type == ASVAL_CMP_EXT_TYPE) {
		if (data == ASVAL_CMP_WILDCARD) {
			*v = (as_val *)&as_cmp_wildcard;
			return 0;
		}
		else if (data == ASVAL_CMP_INF) {
			*v = (as_val *)&as_cmp_inf;
			return 0;
		}
	}

	return -1;
}

static inline int
unpack_boolean(bool b, as_val **v)
{
	*v = (as_val *)as_boolean_new(b);
	return 0;
}

static inline int
unpack_integer_val(int64_t i, as_val **v)
{
	*v = (as_val *)as_integer_new(i);
	return 0;
}

static inline int
unpack_double_val(double d, as_val **v)
{
	*v = (as_val *)as_double_new(d);
	return 0;
}

static int
unpack_blob(as_unpacker *pk, uint32_t size, as_val **val)
{
	unsigned char type = 0;

	if (size != 0) {
		type = pk->buffer[pk->offset++];
		size--;
	}

	if (type == AS_BYTES_STRING) {
		char *v = cf_strndup((const char *)pk->buffer + pk->offset, size);
		*val = (as_val*)as_string_new(v, true);
	}
	else if (type == AS_BYTES_GEOJSON) {
		char *v = cf_strndup((const char *)pk->buffer + pk->offset, size);
		*val = (as_val *)as_geojson_new(v, true);
	}
	else {
		as_bytes *b;

		if (size != 0) {
			unsigned char *buf = cf_malloc(size);

			if (! buf) {
				return -1;
			}

			memcpy(buf, pk->buffer + pk->offset, size);
			b = as_bytes_new_wrap(buf, size, true);

			if (! b) {
				cf_free(buf);
				return -2;
			}
		}
		else {
			b = as_bytes_new_wrap(NULL, 0, false);
		}

		if (! b) {
			return -3;
		}

		b->type = (as_bytes_type) type;
		*val = (as_val *)b;
	}

	if (! *val) {
		return -4;
	}

	pk->offset += size;

	return 0;
}

static int
unpack_list(as_unpacker *pk, uint32_t size, as_val **val)
{
	uint8_t flags = 0;

	// Skip ext element key which is only at the start for metadata.
	if (size != 0 && as_unpack_peek_is_ext(pk)) {
		as_msgpack_ext ext;

		if (as_unpack_ext(pk, &ext) != 0) {
			return -1;
		}

		flags = ext.type;
		size--;
	}

	as_arraylist *list = as_arraylist_new(size, 8);

	if (! list) {
		return -2;
	}

	for (uint32_t i = 0; i < size; i++) {
		as_val *v = NULL;

		if (as_unpack_val(pk, &v) != 0 || ! v) {
			as_arraylist_destroy(list);
			return -3;
		}

		as_arraylist_set(list, i, v);
	}

	*val = (as_val *)list;
	list->_.flags = flags;

	return 0;
}

static int
unpack_map_create_list(as_unpacker *pk, uint32_t size, as_val **val)
{
	// Create list of key value pairs.
	as_arraylist *list = as_arraylist_new(2 * size, 2 * size);

	if (! list) {
		return -1;
	}

	for (uint32_t i = 0; i < size; i++) {
		as_val *k = NULL;
		as_val *v = NULL;

		if (as_unpack_val(pk, &k) != 0) {
			as_arraylist_destroy(list);
			return -2;
		}

		if (as_unpack_val(pk, &v) != 0) {
			as_val_destroy(k);
			as_arraylist_destroy(list);
			return -3;
		}

		if (k && v) {
			as_arraylist_append(list, k);
			as_arraylist_append(list, v);
		}
		else {
			as_val_destroy(k);
			as_val_destroy(v);
		}
	}

	*val = (as_val *)list;

	return 0;
}

static int
unpack_map(as_unpacker *pk, uint32_t size, as_val **val)
{
	uint8_t flags = 0;

	// Skip ext element key which is only at the start for metadata.
	if (size != 0 && as_unpack_peek_is_ext(pk)) {
		as_msgpack_ext ext;

		if (as_unpack_ext(pk, &ext) != 0 || as_unpack_size(pk) < 0) {
			return -1;
		}

		flags = ext.type;
		size--;
	}

	// Check preserve order bit.
	if ((flags & AS_PACKED_MAP_FLAG_PRESERVE_ORDER) != 0) {
		return unpack_map_create_list(pk, size, val);
	}

	as_hashmap *map = as_hashmap_new(size > 32 ? size : 32);

	if (! map) {
		return -2;
	}

	for (uint32_t i = 0; i < size; i++) {
		as_val *k = NULL;
		as_val *v = NULL;

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
			if (as_hashmap_set(map, k, v) != 0) {
				as_val_destroy(k);
				as_val_destroy(v);
				as_hashmap_destroy(map);
				return -5;
			}
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

int
as_unpack_val(as_unpacker *pk, as_val **val)
{
	if (as_unpack_peek_is_ext(pk)) {
		as_unpack_size(pk);
		*val = NULL;
		return 0;
	}

	uint8_t type = pk->buffer[pk->offset++];

	switch (type) {
	case 0xc0: // nil
		return unpack_nil(val);

	case 0xc3: // boolean true
		return unpack_boolean(true, val);
	case 0xc2: // boolean false
		return unpack_boolean(false, val);

	case 0xca: // float
		return unpack_double_val((double)extract_float(pk), val);
	case 0xcb: // double
		return unpack_double_val(extract_double(pk), val);

	case 0xd0: // signed 8 bit integer
		return unpack_integer_val((int64_t)(int8_t)pk->buffer[pk->offset++],
				val);
	case 0xcc: // unsigned 8 bit integer
		return unpack_integer_val((int64_t)pk->buffer[pk->offset++], val);

	case 0xd1: // signed 16 bit integer
		return unpack_integer_val((int64_t)(int16_t)extract_uint16(pk), val);
	case 0xcd: // unsigned 16 bit integer
		return unpack_integer_val((int64_t)extract_uint16(pk), val);

	case 0xd2: // signed 32 bit integer
		return unpack_integer_val((int64_t)(int32_t)extract_uint32(pk), val);
	case 0xce: // unsigned 32 bit integer
		return unpack_integer_val((int64_t)extract_uint32(pk), val);

	case 0xd3: // signed 64 bit integer
	case 0xcf: // unsigned 64 bit integer
		return unpack_integer_val((int64_t)extract_uint64(pk), val);

	case 0xc4:
	case 0xd9: // string/raw bytes with 8 bit header
		return unpack_blob(pk, (uint32_t)pk->buffer[pk->offset++], val);
	case 0xc5:
	case 0xda: // string/raw bytes with 16 bit header
		return unpack_blob(pk, (uint32_t)extract_uint16(pk), val);
	case 0xc6:
	case 0xdb: // string/raw bytes with 32 bit header
		return unpack_blob(pk, extract_uint32(pk), val);

	case 0xdc: // list with 16 bit header
		return unpack_list(pk, (uint32_t)extract_uint16(pk), val);
	case 0xdd: // list with 32 bit header
		return unpack_list(pk, extract_uint32(pk), val);

	case 0xde: // map with 16 bit header
		return unpack_map(pk, (uint32_t)extract_uint16(pk), val);
	case 0xdf: // map with 32 bit header
		return unpack_map(pk, extract_uint32(pk), val);

	case 0xd4: // fixext 1
		return unpack_ext(pk, type, val);

	default:
		if ((type & 0xe0) == 0xa0) { // raw bytes with 8 bit combined header
			return unpack_blob(pk, (uint32_t)(type & 0x1f), val);
		}

		if ((type & 0xf0) == 0x80) { // map with 8 bit combined header
			return unpack_map(pk, (uint32_t)(type & 0x0f), val);
		}

		if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
			return unpack_list(pk, (uint32_t)(type & 0x0f), val);
		}

		if (type < 0x80) { // 8 bit combined unsigned integer
			return unpack_integer_val((int64_t)type, val);
		}

		if (type >= 0xe0) { // 8 bit combined signed integer
			return unpack_integer_val((int64_t)(type & 0x1f) - 32, val);
		}

		return -2;
	}
}

/******************************************************************************
 * Pack direct functions
 ******************************************************************************/

int
as_pack_nil(as_packer *pk)
{
	return pack_byte(pk, 0xc0, false);
}

int
as_pack_bool(as_packer *pk, bool val)
{
	return pack_byte(pk, val ? 0xc3 : 0xc2, false);
}

static int
as_pack_inf_internal(as_packer *pk, bool resize)
{
	uint8_t fixext[3] = {0xD4, ASVAL_CMP_EXT_TYPE, ASVAL_CMP_INF};
	return pack_append(pk, fixext, 3, resize);
}

int
as_pack_cmp_inf(as_packer *pk)
{
	return as_pack_inf_internal(pk, false);
}

static int
as_pack_wildcard_internal(as_packer *pk, bool resize)
{
	uint8_t fixext[3] = {0xD4, ASVAL_CMP_EXT_TYPE, ASVAL_CMP_WILDCARD};
	return pack_append(pk, fixext, 3, resize);
}

int
as_pack_cmp_wildcard(as_packer *pk)
{
	return as_pack_wildcard_internal(pk, false);
}

uint32_t
as_pack_uint64_size(uint64_t val)
{
	if (val < (1ULL << 7)) {
		return 1;
	}

	if (val < (1ULL << 8)) {
		return 2;
	}

	if (val < (1ULL << 16)) {
		return 3;
	}

	if (val < (1ULL << 32)) {
		return 5;
	}

	return 9;
}

uint32_t
as_pack_int64_size(int64_t val)
{
	if (val >= 0) {
		return as_pack_uint64_size((uint64_t)val);
	}

	if (val >= -(1LL << 5)) {
		return 1;
	}

	if (val >= -(1LL << 7)) {
		return 1 + 1;
	}

	if (val >= -(1LL << 15)) {
		return 1 + 2;
	}

	if (val >= -(1LL << 31)) {
		return 1 + 4;
	}

	return 1 + 8;
}

int
as_pack_uint64(as_packer *pk, uint64_t val)
{
	return pack_uint64(pk, val, false);
}

int
as_pack_int64(as_packer *pk, int64_t val)
{
	return pack_int64(pk, val, false);
}

int
as_pack_float(as_packer *pk, float val)
{
	return pack_type_uint32(pk, 0xca, *(uint32_t *)&val, false);
}

int
as_pack_double(as_packer *pk, double val)
{
	return pack_double(pk, val, false);
}

int
as_pack_bytes(as_packer *pk, const uint8_t *buf, uint32_t sz)
{
	int rc = pack_byte_array_header(pk, sz + 1);

	if (rc == 0) {
		rc = pack_byte(pk, AS_BYTES_BLOB, true);
	}

	if (rc == 0) {
		rc = pack_append(pk, buf, sz, false);
	}
	return rc;
}

uint32_t
as_pack_str_size(uint32_t str_sz)
{
	if (str_sz < 32) {
		return 1 + str_sz;
	}

	return as_pack_bin_size(str_sz);
}

uint32_t
as_pack_bin_size(uint32_t buf_sz)
{
	if (buf_sz < (1 << 8)) {
		return 2 + buf_sz;
	}

	if (buf_sz < (1 << 16)) {
		return 3 + buf_sz;
	}

	return 5 + buf_sz;
}

int
as_pack_str(as_packer *pk, const uint8_t *buf, uint32_t sz)
{
	int rc = pack_string_header(pk, sz);

	if (rc == 0 && buf) {
		return pack_append(pk, buf, sz, false);
	}

	return rc;
}

int
as_pack_bin(as_packer *pk, const uint8_t *buf, uint32_t sz)
{
	int rc;

	if (sz < (1 << 8)) {
		rc = pack_type_uint8(pk, 0xc4, (uint8_t)sz, false);
	}
	else if (sz < (1 << 16)) {
		rc = pack_type_uint16(pk, 0xc5, (uint16_t)sz, false);
	}
	else {
		rc = pack_type_uint32(pk, 0xc6, sz, false);
	}

	if (rc == 0 && buf) {
		return pack_append(pk, buf, sz, false);
	}

	return rc;
}

int
as_pack_list_header(as_packer *pk, uint32_t ele_count)
{
	int rc;

	if (ele_count < 16) {
		rc = pack_byte(pk, (uint8_t)(0x90 | ele_count), false);
	}
	else if (ele_count < (1 << 16)) {
		rc = pack_type_uint16(pk, 0xdc, (uint16_t)ele_count, false);
	}
	else {
		rc = pack_type_uint32(pk, 0xdd, ele_count, false);
	}

	return rc;
}

uint32_t
as_pack_list_header_get_size(uint32_t ele_count)
{
	if (ele_count < 16) {
		return 1;
	}
	else if (ele_count < (1 << 16)) {
		return 3;
	}
	else {
		return 5;
	}
}

int
as_pack_map_header(as_packer *pk, uint32_t ele_count)
{
	int rc;

	if (ele_count < 16) {
		rc = pack_byte(pk, (uint8_t)(0x80 | ele_count), false);
	}
	else if (ele_count < (1 << 16)) {
		rc = pack_type_uint16(pk, 0xde, (uint16_t)ele_count, false);
	}
	else {
		rc = pack_type_uint32(pk, 0xdf, ele_count, false);
	}

	return rc;
}

uint32_t
as_pack_ext_header_get_size(uint32_t content_size)
{
	if (content_size == 1 || content_size == 2 || content_size == 4 ||
			content_size == 8 || content_size == 16) {
		return 1 + 1;
	}
	else if (content_size < (1 << 8)) {
		return 1 + 1 + 1;
	}
	else if (content_size < (1 << 16)) {
		return 1 + 2 + 1;
	}

	return 1 + 4 + 1;
}

int
as_pack_ext_header(as_packer *pk, uint32_t content_size, uint8_t type)
{
	int rc;

	if (content_size == 1) {
		rc = pack_byte(pk, 0xd4, false);
	}
	else if (content_size == 2) {
		rc = pack_byte(pk, 0xd5, false);
	}
	else if (content_size == 4) {
		rc = pack_byte(pk, 0xd6, false);
	}
	else if (content_size == 8) {
		rc = pack_byte(pk, 0xd7, false);
	}
	else if (content_size == 16) {
		rc = pack_byte(pk, 0xd8, false);
	}
	else if (content_size < (1 << 8)) {
		rc = pack_type_uint8(pk, 0xc7, (uint8_t)content_size, false);
	}
	else if (content_size < (1 << 16)) {
		rc = pack_type_uint16(pk, 0xc8, (uint16_t)content_size, false);
	}
	else {
		rc = pack_type_uint32(pk, 0xc9, content_size, false);
	}

	if (rc != 0) {
		return rc;
	}

	return pack_byte(pk, type, false);
}

int
as_pack_buf_ext_header(uint8_t *buf, uint32_t size, uint32_t content_size,
		uint8_t type)
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

int
as_pack_append(as_packer *pk, const unsigned char *buf, uint32_t sz)
{
	return pack_append(pk, buf, sz, false);
}

/******************************************************************************
 * Unpack direct functions
 ******************************************************************************/

/**
 * Get size of list with ele_count elements.
 * Assume header already extracted.
 * @return -1 on failure
 */
static int64_t
unpack_list_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		state->index = 0;
		state->map_pair = 0;
		state->len = ele_count;
		state->type = AS_LIST;

		int64_t ret = unpack_size_non_recursive(pk, block, state);

		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t total = 0;

	for (uint32_t i = 0; i < ele_count; i++) {
		int64_t ret = unpack_size_internal(pk, depth);

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
static int64_t
unpack_map_elements_size(as_unpacker *pk, uint32_t ele_count, uint32_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		state->index = 0;
		state->map_pair = 0;
		state->len = ele_count;
		state->type = AS_MAP;

		int64_t ret = unpack_size_non_recursive(pk, block, state);
		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t total = 0;

	for (uint32_t i = 0; i < ele_count; i++) {
		int64_t ret = unpack_size_internal(pk, depth);

		if (ret < 0) {
			return -1;
		}

		total += ret;
		ret = unpack_size_internal(pk, depth);

		if (ret < 0) {
			return -2;
		}

		total += ret;
	}

	return total;
}

static inline as_val_t
bytes_internal_type_to_as_val_t(uint8_t type)
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

as_val_t
as_unpack_peek_type(const as_unpacker *pk)
{
	if (pk->length <= pk->offset) {
		return AS_UNDEF;
	}

	uint8_t type = pk->buffer[pk->offset];

	switch (type) {
	case 0xc0: // nil
		return AS_NIL;

	case 0xc3: // boolean true
	case 0xc2: // boolean false
		return AS_BOOLEAN;

	case 0xd0: // signed 8 bit integer
	case 0xcc: // unsigned 8 bit integer
	case 0xd1: // signed 16 bit integer
	case 0xcd: // unsigned 16 bit integer
	case 0xd2: // signed 32 bit integer
	case 0xce: // unsigned 32 bit integer
	case 0xd3: // signed 64 bit integer
	case 0xcf: // unsigned 64 bit integer
		return AS_INTEGER;

	case 0xca: // float
	case 0xcb: // double
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

	case 0xdc: // list with 16 bit header
	case 0xdd: // list with 32 bit header
		return AS_LIST;

	case 0xde: // map with 16 bit header
	case 0xdf: // map with 32 bit header
		return AS_MAP;

	case 0xd4: {
		uint8_t ext_type = pk->buffer[pk->offset + 1];

		if (ext_type == ASVAL_CMP_EXT_TYPE) {
			uint8_t data = pk->buffer[pk->offset + 2];

			if (data == ASVAL_CMP_WILDCARD) {
				return AS_CMP_WILDCARD;
			}

			if (data == ASVAL_CMP_INF) {
				return AS_CMP_INF;
			}
		}

		return AS_UNDEF;
	}

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

as_val_t
as_unpack_buf_peek_type(const uint8_t *buf, uint32_t size)
{
	const as_unpacker pk = {
			.buffer = buf,
			.offset = 0,
			.length = size,
	};

	return as_unpack_peek_type(&pk);
}

static int64_t
unpack_size_non_recursive(as_unpacker *pk, msgpack_parse_memblock *block,
		msgpack_parse_state *state)
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

		if (unpack_size_internal(pk, 0) < 0) {
			return -5;
		}
	}

	return -6;
}

static inline int64_t
unpack_size_internal(as_unpacker *pk, uint32_t depth)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];

	switch (type) {
	case 0xc0: // nil
	case 0xc3: // boolean true
	case 0xc2: // boolean false
		return 1;

	case 0xd0: // signed 8 bit integer
	case 0xcc: // unsigned 8 bit integer
		pk->offset++;
		return 1 + 1;

	case 0xd1: // signed 16 bit integer
	case 0xcd: // unsigned 16 bit integer
		pk->offset += 2;
		return 1 + 2;

	case 0xca: // float
	case 0xd2: // signed 32 bit integer
	case 0xce: // unsigned 32 bit integer
		pk->offset += 4;
		return 1 + 4;

	case 0xcb: // double
	case 0xd3: // signed 64 bit integer
	case 0xcf: // unsigned 64 bit integer
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
		uint16_t length = extract_uint16(pk);
		pk->offset += length;
		return 1 + 2 + length;
	}

	case 0xc6:
	case 0xdb: { // string/raw bytes with 32 bit header
		uint32_t length = extract_uint32(pk);
		pk->offset += length;
		return 1 + 4 + length;
	}

	case 0xdc: { // list with 16 bit header
		uint16_t length = extract_uint16(pk);
		int64_t ret = unpack_list_elements_size(pk, length, depth);
		if (ret < 0) {
			return -2;
		}
		return 1 + 2 + ret;
	}

	case 0xdd: { // list with 32 bit header
		uint32_t length = extract_uint32(pk);
		int64_t ret = unpack_list_elements_size(pk, length, depth);
		if (ret < 0) {
			return -3;
		}
		return 1 + 4 + ret;
	}

	case 0xde: { // map with 16 bit header
		uint16_t length = extract_uint16(pk);
		int64_t ret = unpack_map_elements_size(pk, length, depth);
		if (ret < 0) {
			return -4;
		}
		return 1 + 2 + ret;
	}

	case 0xdf: { // map with 32 bit header
		uint32_t length = extract_uint32(pk);
		int64_t ret = unpack_map_elements_size(pk, length, depth);
		if (ret < 0) {
			return -5;
		}
		return 1 + 4 + ret;
	}

	case 0xd4: // fixext 1
		pk->offset += 1 + 1;
		return 1 + 1 + 1;
	case 0xd5: // fixext 2
		pk->offset += 1 + 2;
		return 1 + 1 + 2;
	case 0xd6: // fixext 4
		pk->offset += 1 + 4;
		return 1 + 1 + 4;
	case 0xd7: // fixext 8
		pk->offset += 1 + 8;
		return 1 + 1 + 8;
	case 0xd8: // fixext 16
		pk->offset += 1 + 16;
		return 1 + 1 + 16;
	case 0xc7: { // ext 8
		uint8_t length = pk->buffer[pk->offset++];
		pk->offset += 1 + length;
		return 1 + 1 + 1 + length;
	}
	case 0xc8: { // ext 16
		uint16_t length = extract_uint16(pk);
		pk->offset += 1 + length;
		return 1 + 2 + 1 + length;
	}
	case 0xc9: { // ext 32
		uint32_t length = extract_uint32(pk);
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
		int64_t ret = unpack_map_elements_size(pk, type & 0x0f, depth);
		if (ret < 0) {
			return -6;
		}
		return 1 + ret;
	}

	if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
		int64_t ret = unpack_list_elements_size(pk, type & 0x0f, depth);
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

int
as_unpack_boolean(as_unpacker *pk, bool *value)
{
	uint8_t type = pk->buffer[pk->offset++];

	if (type == 0xc3 || type == 0xc2) {
		*value = type == 0xc3;
		return 0;
	}

	return -1;
}

int
as_unpack_nil(as_unpacker *pk)
{
	uint8_t type = pk->buffer[pk->offset++];

	return type == 0xc0 ? 0 : -1;
}

int64_t
as_unpack_size(as_unpacker *pk)
{
	return unpack_size_internal(pk, 0);
}

int64_t
as_unpack_blob_size(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xc4:
	case 0xd9: // string/raw bytes with 8 bit header
		if (size < 1) {
			return -2;
		}
		return (int64_t)pk->buffer[pk->offset++];

	case 0xc5:
	case 0xda: // string/raw bytes with 16 bit header
		if (size < 2) {
			return -3;
		}
		return (int64_t)extract_uint16(pk);

	case 0xc6:
	case 0xdb: // string/raw bytes with 32 bit header
		if (size < 4) {
			return -4;
		}
		return (int64_t)extract_uint32(pk);

	default:
		if ((type & 0xe0) == 0xa0) { // raw bytes with 8 bit combined header
			return type & 0x1f;
		}
		break;
	}

	return -5;
}

int
as_unpack_uint64(as_unpacker *pk, uint64_t *i)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xd0: // signed 8 bit integer
		if (size < 1) {
			return -1;
		}
		*i = (uint64_t)(int8_t)pk->buffer[pk->offset++];
		break;
	case 0xcc: // unsigned 8 bit integer
		if (size < 1) {
			return -2;
		}
		*i = (uint64_t)pk->buffer[pk->offset++];
		break;

	case 0xd1: // signed 16 bit integer
		if (size < 2) {
			return -3;
		}
		*i = (uint64_t)(int16_t)extract_uint16(pk);
		break;
	case 0xcd: // unsigned 16 bit integer
		if (size < 2) {
			return -4;
		}
		*i = (uint64_t)extract_uint16(pk);
		break;

	case 0xd2: // signed 32 bit integer
		if (size < 4) {
			return -5;
		}
		*i = (uint64_t)(int32_t)extract_uint32(pk);
		break;
	case 0xce: // unsigned 32 bit integer
		if (size < 4) {
			return -6;
		}
		*i = (uint64_t)extract_uint32(pk);
		break;

	case 0xd3: // signed 64 bit integer
	case 0xcf: // unsigned 64 bit integer
		if (size < 8) {
			return -7;
		}
		*i = extract_uint64(pk);
		break;

	default:
		if (type < 0x80) { // 8 bit combined unsigned integer
			*i = type;
			break;
		}

		if (type >= 0xe0) { // 8 bit combined signed integer
			*i = (uint64_t)(type & 0x1f) - 32;
			break;
		}

		return -8;
	}

	return 0;
}

int
as_unpack_int64(as_unpacker *pk, int64_t *i)
{
	return as_unpack_uint64(pk, (uint64_t *)i);
}

int
as_unpack_double(as_unpacker *pk, double *x)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xca: // float
		if (size < 4) {
			return -2;
		}
		*x = (double)extract_float(pk);
		break;
	case 0xcb: // double
		if (size < 8) {
			return -3;
		}
		*x = extract_double(pk);
		break;
	default:
		return -4;
	}

	return 0;
}

static inline const uint8_t *
unpack_str_bin(as_unpacker *pk, uint32_t *sz_r)
{
	if (pk->offset >= pk->length) {
		return NULL;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xd9:
	case 0xc4: // str/bin with 8 bit header
		if (size < 1) {
			return NULL;
		}
		*sz_r = (uint32_t)pk->buffer[pk->offset++];
		break;
	case 0xc5:
	case 0xda: // str/bin with 16 bit header
		if (size < 2) {
			return NULL;
		}
		*sz_r = (uint32_t)extract_uint16(pk);
		break;
	case 0xc6:
	case 0xdb: // str/bin with 32 bit header
		if (size < 4) {
			return NULL;
		}
		*sz_r = extract_uint32(pk);
		break;
	default:
		if ((type & 0xe0) == 0xa0) { // str bytes with 8 bit combined header
			*sz_r = (uint32_t)(type & 0x1f);
			break;
		}

		return NULL;
	}

	const uint8_t *buf = &pk->buffer[pk->offset];

	pk->offset += *sz_r;

	if (pk->offset > pk->length) {
		return NULL;
	}

	return buf;
}

const uint8_t *
as_unpack_str(as_unpacker *pk, uint32_t *sz_r)
{
	return unpack_str_bin(pk, sz_r);
}

const uint8_t *
as_unpack_bin(as_unpacker *pk, uint32_t *sz_r)
{
	return unpack_str_bin(pk, sz_r);
}

int
as_unpack_ext(as_unpacker *pk, as_msgpack_ext *ext)
{
	// Need at least 3 bytes.
	if (pk->length - pk->offset < 3) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];

	switch (type) {
	case 0xd4: // fixext 1
		ext->size = 1;
		break;
	case 0xd5: // fixext 2
		ext->size = 2;
		break;
	case 0xd6: // fixext 4
		ext->size = 4;
		break;
	case 0xd7: // fixext 8
		ext->size = 8;
		break;
	case 0xd8: // fixext 16
		ext->size = 16;
		break;
	case 0xc7: // ext 8
		ext->size = (uint32_t)pk->buffer[pk->offset++];
		break;
	case 0xc8: // ext 16
		ext->size = extract_uint16(pk);
		break;
	case 0xc9: // ext 32
		// Need at least 4 more bytes.
		if (pk->length - pk->offset < 4) {
			return -2;
		}
		ext->size = extract_uint32(pk);
		break;
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

int64_t
as_unpack_buf_list_element_count(const uint8_t *buf, uint32_t size)
{
	as_unpacker pk = {
			.buffer = buf,
			.offset = 0,
			.length = size,
	};

	return as_unpack_list_header_element_count(&pk);
}

int64_t
as_unpack_list_header_element_count(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xdc: // list with 16 bit header
		if (size < 2) {
			return -2;
		}
		return extract_uint16(pk);
	case 0xdd: // list with 32 bit header
		if (size < 4) {
			return -3;
		}
		return (int64_t)extract_uint32(pk);
	default:
		break;
	}

	if ((type & 0xf0) == 0x90) { // list with 8 bit combined header
		return (int64_t)(type & 0x0f);
	}

	return -4;
}

/******************************************************************************
 * Compare direct functions
 ******************************************************************************/

int64_t
as_unpack_map_header_element_count(as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return -1;
	}

	uint8_t type = pk->buffer[pk->offset++];
	uint32_t size = pk->length - pk->offset;

	switch (type) {
	case 0xde: // map with 16 bit header
		if (size < 2) {
			return -2;
		}
		return extract_uint16(pk);
	case 0xdf: // map with 32 bit header
		if (size < 4) {
			return -3;
		}
		return extract_uint32(pk);
	default:
		break;
	}

	if ((type & 0xf0) == 0x80) { // map with 8 bit combined header
		return type & 0x0f;
	}

	return -4;
}

int64_t
as_unpack_buf_map_element_count(const uint8_t *buf, uint32_t size)
{
	as_unpacker pk = {
			.buffer = buf,
			.offset = 0,
			.length = size,
	};

	return as_unpack_map_header_element_count(&pk);
}

bool
as_unpack_peek_is_ext(const as_unpacker *pk)
{
	if (pk->offset >= pk->length) {
		return false;
	}

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

static inline msgpack_compare_t
msgpack_compare_int(as_unpacker *pk1, as_unpacker *pk2)
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

static inline msgpack_compare_t
msgpack_compare_double(as_unpacker *pk1, as_unpacker *pk2)
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

static inline int64_t
msgpack_get_blob_len(as_unpacker *pk)
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

static msgpack_compare_t
msgpack_compare_blob_internal(as_unpacker *pk1, uint32_t len1, as_unpacker *pk2,
	uint32_t len2)
{
	uint32_t minlen = (len1 < len2) ? len1 : len2;

	uint32_t offset1 = pk1->offset;
	uint32_t offset2 = pk2->offset;

	pk1->offset += len1;
	pk2->offset += len2;

	for (uint32_t i = 0; i < minlen; i++) {
		unsigned char c1 = pk1->buffer[offset1++];
		unsigned char c2 = pk2->buffer[offset2++];

		MSGPACK_COMPARE_RET_LESS_OR_GREATER(c1, c2);
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(len1, len2);

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t
msgpack_compare_blob(as_unpacker *pk1, as_unpacker *pk2)
{
	int64_t len1 = msgpack_get_blob_len(pk1);
	int64_t len2 = msgpack_get_blob_len(pk2);

	if (len1 < 0 || len2 < 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	return msgpack_compare_blob_internal(pk1, (uint32_t)len1, pk2, (uint32_t)len2);
}

static inline msgpack_compare_t
msgpack_compare_int64_t(int64_t x1, int64_t x2)
{
	MSGPACK_COMPARE_RET_LESS_OR_GREATER(x1, x2);

	return MSGPACK_COMPARE_EQUAL;
}

// Skip n vals.
static bool
msgpack_skip(as_unpacker *pk, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (as_unpack_size(pk) < 0) {
			return false;
		}
	}

	return true;
}

static bool
msgpack_compare_unwind(as_unpacker *pk1, as_unpacker *pk2,
		const msgpack_parse_state *state)
{
	if (state->type == AS_LIST) {
		if (! msgpack_skip(pk1, state->len1 - state->index)) {
			return false;
		}

		if (! msgpack_skip(pk2, state->len2 - state->index)) {
			return false;
		}
	}
	else if (state->type == AS_MAP) {
		if (! msgpack_skip(pk1,
				2 * (state->len1 - state->index) - state->map_pair)) {
			return false;
		}

		if (! msgpack_skip(pk2,
				2 * (state->len2 - state->index) - state->map_pair)) {
			return false;
		}
	}

	return true;
}

static bool
msgpack_compare_unwind_all(as_unpacker *pk1, as_unpacker *pk2,
		msgpack_parse_memblock **block)
{
	if ((*block)->count == 0) {
		return true;
	}

	const msgpack_parse_state *state = &(*block)->buffer[(*block)->count - 1];

	while (true) {
		if (! msgpack_compare_unwind(pk1, pk2, state)) {
			return false;
		}

		if (! msgpack_parse_memblock_has_prev(*block)) {
			break;
		}

		state = msgpack_parse_memblock_prev(block);
	}

	return true;
}

static msgpack_compare_t
msgpack_compare_list(as_unpacker *pk1, as_unpacker *pk2, size_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		if (! msgpack_parse_state_list_cmp_init(state, pk1, pk2)) {
			msgpack_parse_memblock_destroy(block);
			return MSGPACK_COMPARE_ERROR;
		}

		msgpack_compare_t ret = msgpack_compare_non_recursive(pk1, pk2, &block,
				state);

		if (ret == MSGPACK_COMPARE_ERROR ||
				! msgpack_compare_unwind_all(pk1, pk2, &block)) {
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
		as_val_t type;
		msgpack_compare_t ret = msgpack_compare_internal(pk1, pk2, depth,
				&type);

		if (ret != MSGPACK_COMPARE_EQUAL || type == AS_CMP_WILDCARD) {
			if (! msgpack_skip(pk1, len1 - i - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}

			if (! msgpack_skip(pk2, len2 - i - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}

			return ret;
		}
	}

	if (! msgpack_skip(pk1, len1 - minlen)) {
		return MSGPACK_COMPARE_ERROR;
	}

	if (! msgpack_skip(pk2, len2 - minlen)) {
		return MSGPACK_COMPARE_ERROR;
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(len1, len2);

	return MSGPACK_COMPARE_EQUAL;
}

static msgpack_compare_t
msgpack_compare_map(as_unpacker *pk1, as_unpacker *pk2, size_t depth)
{
	if (++depth > MSGPACK_COMPARE_MAX_DEPTH) {
		msgpack_parse_memblock *block = msgpack_parse_memblock_create(NULL);
		msgpack_parse_state *state = msgpack_parse_memblock_next(&block);

		if (! msgpack_parse_state_map_cmp_init(state, pk1, pk2)) {
			msgpack_parse_memblock_destroy(block);
			return MSGPACK_COMPARE_ERROR;
		}

		if (state->default_compare_type != MSGPACK_COMPARE_EQUAL) {
			msgpack_parse_memblock_destroy(block);
			return state->default_compare_type;
		}

		msgpack_compare_t ret = msgpack_compare_non_recursive(pk1, pk2, &block,
				state);

		if (ret == MSGPACK_COMPARE_ERROR ||
				! msgpack_compare_unwind_all(pk1, pk2, &block)) {
			msgpack_parse_memblock_destroy(block);
			return MSGPACK_COMPARE_ERROR;
		}

		msgpack_parse_memblock_destroy(block);

		return ret;
	}

	int64_t len1 = as_unpack_map_header_element_count(pk1);
	int64_t len2 = as_unpack_map_header_element_count(pk2);
	int64_t minlen = (len1 < len2) ? len1 : len2;

	if (minlen < 0) {
		return MSGPACK_COMPARE_ERROR;
	}

	if (len1 != len2) {
		if (! msgpack_skip(pk1, len1)) {
			return MSGPACK_COMPARE_ERROR;
		}

		if (! msgpack_skip(pk2, len2)) {
			return MSGPACK_COMPARE_ERROR;
		}

		MSGPACK_COMPARE_RET_LESS_OR_GREATER(len1, len2);
	}

	for (int64_t i = 0; i < minlen; i++) {
		as_val_t type;
		msgpack_compare_t ret = msgpack_compare_internal(pk1, pk2, depth,
				&type);

		if (ret != MSGPACK_COMPARE_EQUAL|| type == AS_CMP_WILDCARD) {
			if (! msgpack_skip(pk1, 2 * (len1 - i) - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}

			if (! msgpack_skip(pk2, 2 * (len2 - i) - 1)) {
				return MSGPACK_COMPARE_ERROR;
			}

			return ret;
		}

		ret = msgpack_compare_internal(pk1, pk2, depth, &type);

		if (ret != MSGPACK_COMPARE_EQUAL || type == AS_CMP_WILDCARD) {
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

static inline msgpack_compare_t
msgpack_peek_compare_type(const as_unpacker *pk1, const as_unpacker *pk2,
		as_val_t *type)
{
	int n1 = pk1->length - pk1->offset;
	int n2 = pk2->length - pk2->offset;

	*type = AS_UNDEF;

	if (n1 == 0 || n2 == 0) {
		MSGPACK_COMPARE_RET_LESS_OR_GREATER(n1, n2);

		return MSGPACK_COMPARE_END;
	}

	as_val_t type1 = as_unpack_peek_type(pk1);
	as_val_t type2 = as_unpack_peek_type(pk2);

	if (type1 == AS_UNDEF || type2 == AS_UNDEF) {
		return MSGPACK_COMPARE_ERROR;
	}

	if (type1 == AS_CMP_WILDCARD || type2 == AS_CMP_WILDCARD) {
		*type = AS_CMP_WILDCARD;

		return MSGPACK_COMPARE_EQUAL;
	}

	MSGPACK_COMPARE_RET_LESS_OR_GREATER(type1, type2);

	*type = type1;

	return MSGPACK_COMPARE_EQUAL;
}

static msgpack_compare_t
msgpack_compare_non_recursive(as_unpacker *pk1, as_unpacker *pk2,
		msgpack_parse_memblock **block, msgpack_parse_state *state)
{
	while (state) {
		while (state->index >= state->len) {
			if (! msgpack_compare_unwind(pk1, pk2, state)) {
				return MSGPACK_COMPARE_ERROR;
			}

			if (! msgpack_parse_memblock_has_prev(*block)) {
				return state->default_compare_type;
			}

			uint32_t len1 = state->len1;
			uint32_t len2 = state->len2;

			state = msgpack_parse_memblock_prev(block);

			MSGPACK_COMPARE_RET_LESS_OR_GREATER(len1, len2);
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

		if (ret == MSGPACK_COMPARE_ERROR || ret == MSGPACK_COMPARE_END) {
			return MSGPACK_COMPARE_ERROR;
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
			state = msgpack_parse_memblock_next(block);

			if (! msgpack_parse_state_list_cmp_init(state, pk1, pk2)) {
				return MSGPACK_COMPARE_ERROR;
			}

			continue;
		}

		if (type == AS_MAP) {
			state = msgpack_parse_memblock_next(block);

			if (! msgpack_parse_state_map_cmp_init(state, pk1, pk2)) {
				return MSGPACK_COMPARE_ERROR;
			}

			MSGPACK_COMPARE_RET_LESS_OR_GREATER(state->len1, state->len2);
			continue;
		}

		if (type == AS_CMP_WILDCARD) {
			if (! msgpack_skip(pk1, 1) || ! msgpack_skip(pk2, 1)) {
				return MSGPACK_COMPARE_ERROR;
			}

			if (! msgpack_compare_unwind(pk1, pk2, state)) {
				return MSGPACK_COMPARE_ERROR;
			}

			if (! msgpack_parse_memblock_has_prev(*block)) {
				return MSGPACK_COMPARE_EQUAL;
			}

			state = msgpack_parse_memblock_prev(block);
			continue;
		}

		ret = msgpack_compare_type(pk1, pk2, type, 0);

		if (ret != MSGPACK_COMPARE_EQUAL) {
			return ret;
		}
	}

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t
msgpack_compare_type(as_unpacker *pk1, as_unpacker *pk2, as_val_t type,
		size_t depth)
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
	case AS_CMP_WILDCARD:
	case AS_CMP_INF:
		pk1->offset += 3;
		pk2->offset += 3;
		break;
	default:
		return MSGPACK_COMPARE_ERROR;
	}

	return MSGPACK_COMPARE_EQUAL;
}

static inline msgpack_compare_t
msgpack_compare_internal(as_unpacker *pk1, as_unpacker *pk2, size_t depth,
		as_val_t *type)
{
	msgpack_compare_t ret = msgpack_peek_compare_type(pk1, pk2, type);

	if (ret == MSGPACK_COMPARE_ERROR || ret == MSGPACK_COMPARE_END) {
		return MSGPACK_COMPARE_ERROR;
	}

	if (ret != MSGPACK_COMPARE_EQUAL || *type == AS_CMP_WILDCARD) {
		if (as_unpack_size(pk1) < 0) {
			return MSGPACK_COMPARE_ERROR;
		}

		if (as_unpack_size(pk2) < 0) {
			return MSGPACK_COMPARE_ERROR;
		}

		return ret;
	}

	return msgpack_compare_type(pk1, pk2, *type, depth);
}

msgpack_compare_t
as_unpack_buf_compare(const uint8_t *buf1, uint32_t size1, const uint8_t *buf2,
		uint32_t size2)
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
	as_val_t type;

	return msgpack_compare_internal(&pk1, &pk2, 0, &type);
}

msgpack_compare_t
as_unpack_compare(as_unpacker *pk1, as_unpacker *pk2)
{
	as_val_t type;
	return msgpack_compare_internal(pk1, pk2, 0, &type);
}
