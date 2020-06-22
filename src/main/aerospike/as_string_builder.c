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
#include <aerospike/as_string_builder.h>
#include <citrusleaf/alloc.h>
#include <string.h>
#include <stdio.h>

extern const char as_hex_chars[];

void
as_string_builder_init(as_string_builder* sb, uint32_t capacity, bool resize)
{
	sb->data = (char*)cf_malloc(capacity);
	sb->data[0] = 0;
	sb->capacity = capacity;
	sb->length = 0;
	sb->resize = resize;
	sb->free = true;
}

void
as_string_builder_destroy(as_string_builder* sb)
{
	if (sb->free) {
		cf_free(sb->data);
		sb->data = 0;
	}
}

static bool
as_sb_increase_capacity(as_string_builder* sb, uint32_t min_capacity)
{
	uint32_t capacity = sb->capacity * 2;

	if (capacity < min_capacity) {
		capacity = min_capacity;
	}

	if (sb->free) {
		// String already allocated on heap.  Realloc.
		char* data = cf_realloc(sb->data, capacity);
		
		if (data) {
			sb->data = data;
			sb->capacity = capacity;
			return true;
		}
	}
	else {
		// String allocated on stack.  Transfer to heap.
		char* data = cf_malloc(capacity);
		
		if (data) {
			memcpy(data, sb->data, sb->length);
			data[sb->length] = 0;
			sb->data = data;
			sb->capacity = capacity;
			sb->free = true;
			return true;
		}
	}
	return false;
}

static bool
as_sb_expand(as_string_builder* sb, const char* src)
{
	uint32_t remaining_len = (uint32_t)strlen(src);
	uint32_t min_capacity = sb->length + remaining_len + 1;

	if (! as_sb_increase_capacity(sb, min_capacity)) {
		return false;
	}

	memcpy(&sb->data[sb->length], src, remaining_len);
	sb->length += remaining_len;
	sb->data[sb->length] = 0;
	return true;
}

bool
as_string_builder_append(as_string_builder* sb, const char* src)
{
	char* trg = &sb->data[sb->length];

	while (*src) {
		if (++sb->length < sb->capacity) {
			*trg++ = *src++;
			continue;
		}

		// Optimistic increase of length failed. Roll it back.
		sb->length--;

		if (sb->resize) {
			return as_sb_expand(sb, src);
		}
		else {
			*trg = 0;
			return false;
		}
	}
	*trg = 0;
	return true;
}

bool
as_string_builder_append_char(as_string_builder* sb, char value)
{
	if (sb->length + 1 < sb->capacity) {
		sb->data[sb->length++] = value;
		sb->data[sb->length] = 0;
		return true;
	}
	
	if (sb->resize) {
		char buf[] = {value, '\0'};
		return as_sb_expand(sb, buf);
	}
	return false;
}

static inline bool
as_sb_append_char(as_string_builder* sb, char value)
{
	if (sb->length + 1 < sb->capacity) {
		sb->data[sb->length++] = value;
		sb->data[sb->length] = 0;
		return true;
	}
	return false;
}

static bool
as_sb_append_byte(as_string_builder* sb, uint8_t b)
{
	if (sb->length + 3 < sb->capacity) {
		sb->data[sb->length++] = as_hex_chars[(b >> 4) & 0xf];
		sb->data[sb->length++] = as_hex_chars[b & 0xf];
		sb->data[sb->length++] = ' ';
		sb->data[sb->length] = 0;
		return true;
	}
	return false;
}

bool
as_string_builder_append_bytes(as_string_builder* sb, uint8_t* src, uint32_t size)
{
	if (sb->resize) {
		// Ensure buffer is large enough to receive bytes.
		// min capacity = old length + 3 chars per byte including separator + 2 bracket chars
		// - 1 extra separator + 1 null byte.
		uint32_t min_capacity = sb->length + (size * 3) + 2;

		if (min_capacity > sb->capacity) {
			if (! as_sb_increase_capacity(sb, min_capacity)) {
				return false;
			}
		}

		char* trg = &sb->data[sb->length];
		*trg++ = '[';

		for (uint32_t i = 0; i < size; i++) {
			uint8_t b = src[i];
			*trg++ = as_hex_chars[(b >> 4) & 0xf];
			*trg++ = as_hex_chars[b & 0xf];
			*trg++ = ' ';
		}
		trg--; // truncate last separator
		*trg++ = ']';
		*trg = 0;
		sb->length = (int)(trg - sb->data);
	}
	else {
		if (! as_sb_append_char(sb, '[')) {
			return false;
		}

		for (uint32_t i = 0; i < size; i++) {
			if (! as_sb_append_byte(sb, src[i])) {
				return false;
			}
		}
		sb->length--; // truncate last separator

		if (! as_sb_append_char(sb, ']')) {
			return false;
		}
	}
	return true;
}

bool
as_string_builder_append_int(as_string_builder* sb, int val)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d", val);
	return as_string_builder_append(sb, buf);
}

bool
as_string_builder_append_uint(as_string_builder* sb, uint32_t val)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%u", val);
	return as_string_builder_append(sb, buf);
}
