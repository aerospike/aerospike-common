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
#include <aerospike/as_string_builder.h>
#include <citrusleaf/alloc.h>
#include <string.h>

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
as_string_builder_append_increase(as_string_builder* sb, const char* src)
{
	uint32_t remaining_len = (uint32_t)strlen(src);
	uint32_t capacity = sb->capacity * 2;
	uint32_t initial_len = sb->capacity - 1;
	uint32_t total_len = initial_len + remaining_len;
	uint32_t capacity_min = total_len + 1;
	
	if (capacity < capacity_min) {
		capacity = capacity_min;
	}
	
	if (sb->free) {
		// String already allocated on heap.  Realloc.
		char* data = cf_realloc(sb->data, capacity);
		
		if (data) {
			memcpy(&data[initial_len], src, remaining_len);
			data[total_len] = 0;
			sb->data = data;
			sb->capacity = capacity;
			sb->length = total_len;
			return true;
		}
	}
	else {
		// String allocated on stack.  Transfer to heap.
		char* data = cf_malloc(capacity);
		
		if (data) {
			memcpy(data, sb->data, initial_len);
			memcpy(&data[initial_len], src, remaining_len);
			data[total_len] = 0;
			sb->data = data;
			sb->capacity = capacity;
			sb->length = total_len;
			sb->free = true;
			return true;
		}
	}
	return false;
}

bool
as_string_builder_append(as_string_builder* sb, const char* src)
{
	char* trg = &sb->data[sb->length];
	uint32_t max = sb->capacity;
	uint32_t i = sb->length + 1;
	
	while (*src) {
		if (i >= max) {
			if (sb->resize) {
				return as_string_builder_append_increase(sb, src);
			}
			else {
				*trg = 0;
				sb->length = i - 1;
				return false;
			}
		}
		*trg++ = *src++;
		i++;
	}
	*trg = 0;
	sb->length = i - 1;
	return true;
}

bool
as_string_builder_append_char(as_string_builder* sb, char value)
{
	if (sb->length + 1 < sb->capacity) {
		sb->data[sb->length++] = value;
		return true;
	}
	
	if (sb->resize) {
		char buf[] = {value, '\0'};
		return as_string_builder_append_increase(sb, buf);
	}
	return false;
}
