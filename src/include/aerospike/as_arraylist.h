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

/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_list_s;

/**
 *	An dynamic array implementation for `as_list`.
 *
 *	To use the list, you can either initialize a stack allocated list, 
 *	using `as_arraylist_init()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_list list;
 *	as_arraylist_init(&list, 2, 0);
 *	~~~~~~~~~~
 *
 *	Or you can create a new heap allocated list using `as_arraylist_new()`:
 *
 *	~~~~~~~~~~{.c}
 *	as_list * list = as_arraylist_new(2, 0);
 *	~~~~~~~~~~
 *
 *	When you are finished using the list, then you should release the list and
 *	associated resources, using `as_list_destroy()`:
 *	
 *	~~~~~~~~~~{.c}
 *	as_list_destroy(list);
 *	~~~~~~~~~~
 *
 *	@see as_list
 *
 *	@implements as_list
 */
typedef struct as_arraylist_s {

	/**
	 * Number of elements to add, when capacity is reached.
	 * If 0 (zero), then capacity can't be expanded.
	 */
	uint32_t block_size;

	/**
	 * The total number of slots
	 */
	uint32_t capacity;

	/**
	 * The number of slots containing elements
	 */
	uint32_t size;

	/**
	 * The elements of the list
	 */
	as_val ** elements;

	
} as_arraylist;

/**
 * Status codes for various as_arraylist operations.
 */
typedef enum as_arraylist_status_e {
	
	/**
	 *	Normal operation
	 */
	AS_ARRAYLIST_OK         = 0,

	/**
	 *	Unable to expand capacity: allocation failed
	 */
	AS_ARRAYLIST_ERR_ALLOC  = 1,

	/**
	 *	Unable to expand capacity: block_size==0
	 */
	AS_ARRAYLIST_ERR_MAX    = 2

} as_arraylist_status;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize a stack allocated `as_list` as an `as_arraylist`.
 *
 *	~~~~~~~~~~{.c}
 *	as_list list;
 *	as_arraylist_init(&list, 2, 0);
 *	~~~~~~~~~~
 *
 *	When you are finished using the list, you should release the resources
 *	via `as_list_destroy()`.
 *	
 *	~~~~~~~~~~{.c}
 *	as_list_destroy(&list);
 *	~~~~~~~~~~
 *	
 *	@param list 		The as_list to initialize
 *	@param capacity		The number of elements to allocate to the list.
 *	@param block_size	The number of elements to grow the list by, when the 
 *						capacity has been reached.
 *
 *	@return On success, the initialized list. Otherwise NULL.
 */
struct as_list_s * as_arraylist_init(struct as_list_s * list, uint32_t capacity, uint32_t block_size);

/**
 *	Create and initialize a heap allocated `as_list` as an `as_arraylist`.
 *
 *	~~~~~~~~~~{.c}
 *	as_list * list = as_arraylist_new(2, 0);
 *	~~~~~~~~~~
 *
 *	When you are finished using the list, you should release the resources
 *	via `as_list_destroy()`.
 *
 *	~~~~~~~~~~{.c}
 *	as_list_destroy(list);
 *	~~~~~~~~~~
 *
 *	@param capacity		The number of elements to allocate to the list.
 *	@param block_size	The number of elements to grow the list by, when the 
 *						capacity has been reached.
 *  
 *	@return On success, the new list. Otherwise NULL.
 */
struct as_list_s * as_arraylist_new(uint32_t capacity, uint32_t block_size);

/**
 *	Destroy the `as_list` and release resources.
 *
 *	~~~~~~~~~~{.c}
 *	as_arraylist_destroy(list);
 *	~~~~~~~~~~
 *
 *	@param list	The list to destroy.
 */
void as_arraylist_destroy(struct as_list_s * list);
