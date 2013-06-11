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
 * A node in a linked list. The head contains the value, 
 * the tail is the remainder of the list. The tails must be a linkedlist.
 */
struct as_linkedlist_s {
	
	/**
	 * The first value
	 */
	as_val * head;
	
	/**
	 * The remaining list
	 */
	struct as_list_s * tail;
};

typedef struct as_linkedlist_s as_linkedlist;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Initialize a list as a linkedlist.
 */
struct as_list_s * as_linkedlist_init(struct as_list_s *, as_val *, struct as_list_s *);

/**
 * Create a new list as a linkedlist.
 */
struct as_list_s * as_linkedlist_new(as_val *, struct as_list_s *);

/**
 * Free the list and associated resources.
 */
void as_linkedlist_destroy(struct as_list_s *);
