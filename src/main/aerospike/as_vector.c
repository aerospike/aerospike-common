/******************************************************************************
 *	Copyright 2008-2014 by Aerospike.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy 
 *	of this software and associated documentation files (the "Software"), to 
 *	deal in the Software without restriction, including without limitation the 
 *	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 *	sell copies of the Software, and to permit persons to whom the Software is 
 *	furnished to do so, subject to the following conditions:
 * 
 *	The above copyright notice and this permission notice shall be included in 
 *	all copies or substantial portions of the Software.
 * 
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *	IN THE SOFTWARE.
 *****************************************************************************/
#include <aerospike/as_vector.h>

#define FLAGS_HEAP 1
#define FLAGS_CREATED 2

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
