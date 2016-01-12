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
#include <aerospike/as_vector.h>

#define FLAGS_HEAP 1
#define FLAGS_CREATED 2

void
as_vector_init(as_vector* vector, uint32_t item_size, uint32_t capacity)
{
	vector->list = cf_malloc(capacity * item_size);
	vector->capacity = capacity;
	vector->item_size = item_size;
	vector->size = 0;
	vector->flags = 1;
}

as_vector*
as_vector_create(uint32_t item_size, uint32_t capacity)
{
	as_vector* vector = cf_malloc(sizeof(as_vector));
	as_vector_init(vector, item_size, capacity);
	vector->flags = FLAGS_HEAP | FLAGS_CREATED;
	return vector;
}

void
as_vector_destroy(as_vector* vector)
{
	if (vector->flags & FLAGS_HEAP) {
		cf_free(vector->list);
		
		if (vector->flags & FLAGS_CREATED) {
			cf_free(vector);
		}
	}
}

void
as_vector_increase_capacity(as_vector* vector)
{
	if (vector->flags & FLAGS_HEAP) {
		vector->capacity *= 2;
		vector->list = cf_realloc(vector->list, vector->capacity * vector->item_size);
	}
	else {
		uint32_t capacity = vector->capacity * 2;
		void* tmp = cf_malloc(capacity * vector->item_size);
		memcpy(tmp, vector->list, vector->capacity * vector->item_size);
		vector->list = tmp;
		vector->capacity = capacity;
		vector->flags |= FLAGS_HEAP;
	}
}

bool
as_vector_append_unique(as_vector* vector, void* value)
{
	char* item = vector->list;
	for (uint32_t i = 0; i < vector->size; i++) {
		if (memcmp(item, value, vector->item_size) == 0) {
			return false;
		}
		item += vector->item_size;
	}
	as_vector_append(vector, value);
	return true;
}

void*
as_vector_to_array(as_vector* vector, uint32_t* size)
{
	size_t len = vector->size * vector->item_size;
	void* array = cf_malloc(len);
	memcpy(array, vector->list, len);
	*size = vector->size;
	return array;
}
