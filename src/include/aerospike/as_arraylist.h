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

#include <aerospike/as_list.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_arraylist_source_s as_arraylist_source;
typedef struct as_arraylist_iterator_source_s as_arraylist_iterator_source;

enum as_arraylist_status {
    AS_ARRAYLIST_OK         = 0,
    AS_ARRAYLIST_ERR_ALLOC  = 1,
    AS_ARRAYLIST_ERR_MAX    = 2
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_list_hooks      as_arraylist_list_hooks;
extern const as_iterator_hooks  as_arraylist_iterator_hooks;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list *    	as_arraylist_init(as_list *, uint32_t capacity, uint32_t block_size);
as_list *    	as_arraylist_new(uint32_t capacity, uint32_t block_size);
void               as_arraylist_destroy(as_list *);
