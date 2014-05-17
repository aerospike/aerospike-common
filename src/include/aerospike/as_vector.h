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
#pragma once

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_types.h>
#include <string.h>

/******************************************************************************
 *	TYPES
 *****************************************************************************/

/**
 *	A fast, non thread safe dynamic array implementation.
 *  as_vector is not part of the generic as_val family.
 */
typedef struct as_vector_s {
	/**
	 *	The items of the vector.
	 */
	void* list;

	/**
	 *	The total number items allocated.
	 */
	uint32_t capacity;
	
	/**
	 *	The number of items used.
	 */
	uint32_t size;

	/**
	 *	The size of a single item.
	 */
	uint32_t item_size;
	
	/**
	 *	Internal vector flags.
	 */
	uint32_t flags;
} as_vector;

/******************************************************************************
 *	MACROS
 ******************************************************************************/

/**
 *	Initialize a stack allocated as_vector, with item storage on the stack. 
 *  as_vector_inita() will transfer stack memory to the heap if a resize is
 *  required.
 */
#define as_vector_inita(__vector, __item_size, __capacity)\
	(__vector)->list = alloca((__capacity) * (__item_size));\
	(__vector)->capacity = __capacity;\
	(__vector)->item_size = __item_size;\
	(__vector)->size = 0;\
	(__vector)->flags = 0;

/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated as_vector, with item storage on the heap.
 */
static inline void
as_vector_init(as_vector* vector, uint32_t item_size, uint32_t capacity)
{
	vector->list = cf_malloc(capacity * item_size);
	vector->capacity = capacity;
	vector->item_size = item_size;
	vector->size = 0;
	vector->flags = 1;
}

/**
 *	Create a heap allocated as_vector, with item storage on the heap.
 */
as_vector*
as_vector_create(uint32_t item_size, uint32_t capacity);

/**
 *	Free vector.
 */
void
as_vector_destroy(as_vector* vector);

/**
 *	Empty vector without altering data.
 */
static inline void
as_vector_clear(as_vector* vector)
{
	vector->size = 0;
}

/**
 *  Get pointer to item given index.
 */
static inline void*
as_vector_get(as_vector* vector, uint32_t index)
{
	return vector->list + (vector->item_size * index);
}

/**
 *  Get pointer to item pointer given index.
 */
static inline void*
as_vector_get_ptr(as_vector* vector, uint32_t index)
{
	return *(void**)(vector->list + (vector->item_size * index));
}

/**
 *  Double vector capacity.
 */
void
as_vector_increase_capacity(as_vector* vector);

/**
 *  Set item in vector.
 */
static inline void
as_vector_set(as_vector* vector, uint32_t index, void* value)
{
	memcpy(vector->list + (index * vector->item_size), value, vector->item_size);
}

/**
 *  Append item to vector.
 */
static inline void
as_vector_append(as_vector* vector, void* value)
{
	if (vector->size >= vector->capacity) {
		as_vector_increase_capacity(vector);
	}
	memcpy(vector->list + (vector->size * vector->item_size), value, vector->item_size);
	vector->size++;
}

/**
 *  Append item to vector if it doesn't already exist.
 */
bool
as_vector_append_unique(as_vector* vector, void* value);

/**
 *  Move item row position in vector.
 */
static inline void
as_vector_move(as_vector* vector, uint32_t source, uint32_t target)
{
	memcpy(vector->list + (target * vector->item_size), vector->list + (source * vector->item_size), vector->item_size);
}
