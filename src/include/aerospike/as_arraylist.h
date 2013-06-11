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

#pragma once

#include <aerospike/as_val.h>

/*****************************************************************************
 * TYPES
 *****************************************************************************/

struct as_list_s;

/**
 * An dynamic array implementation for as_list.
 */
struct as_arraylist_s {
	/**
	 * The elements of the list
	 */
	as_val ** elements;
	/**
	 * The number of slots containing elements
	 */
	uint32_t size;

	/**
	 * The total number of slots
	 */
	uint32_t capacity;

	/**
	 * Number of elements to add, when capacity is reached.
	 * If 0 (zero), then capacity can't be expanded.
	 */
	uint32_t block_size;
};

typedef struct as_arraylist_s as_arraylist;

/**
 * Status codes for various as_arraylist operations.
 */
enum as_arraylist_status_e {
	
	// Normal operation
	AS_ARRAYLIST_OK         = 0,

	// Unable to expand capacity: allocation failed
	AS_ARRAYLIST_ERR_ALLOC  = 1,

	// Unable to expand capacity: block_size==0
	AS_ARRAYLIST_ERR_MAX    = 2
};

typedef enum as_arraylist_status_e as_arraylist_status;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Initialize a list as a dynamic array.
 */
struct as_list_s * as_arraylist_init(struct as_list_s *, uint32_t capacity, uint32_t block_size);

/**
 * Create a new list as a dynamic array.
 */
struct as_list_s * as_arraylist_new(uint32_t capacity, uint32_t block_size);

/**
 * Free the list and associated resources.
 */
void as_arraylist_destroy(struct as_list_s *);
