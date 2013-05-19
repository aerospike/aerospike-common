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

struct as_arraylist_s {
    as_val **   elements;       // An allocated area for ptrs to list elements
    uint32_t    size;           // The current array size (element count)
    uint32_t    capacity;       // The total array size (max element count)
    uint32_t    block_size;     // The unit of allocation (e.g. 8 elements)
                                // Note that block_size == 0 means no more
                                // can be allocated
};

typedef struct as_arraylist_s as_arraylist;

enum as_arraylist_status_e {
    AS_ARRAYLIST_OK         = 0,
    AS_ARRAYLIST_ERR_ALLOC  = 1,
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
