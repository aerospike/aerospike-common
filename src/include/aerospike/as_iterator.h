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

#include <stdbool.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <aerospike/as_linkedlist_iterator.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_hashmap_iterator.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

struct as_iterator_hooks_s;

/**
 * Iterator Data
 */
union as_iterator_data_u {
    as_linkedlist_iterator  linkedlist;
    as_arraylist_iterator   arraylist;
    as_hashmap_iterator     hashmap;
    void *                  generic;
};

typedef union as_iterator_data_u as_iterator_data;

/**
 * Iterator Object
 */
struct as_iterator_s {
    bool                                is_malloc;
    union as_iterator_data_u            data;
    const struct as_iterator_hooks_s *  hooks;
};

typedef struct as_iterator_s as_iterator;

/**
 * Iterator Function Hooks
 */
struct as_iterator_hooks_s {
    void      (* const destroy)(as_iterator *);
    bool      (* const has_next)(const as_iterator *);
    as_val *  (* const next)(as_iterator *);
};

typedef struct as_iterator_hooks_s as_iterator_hooks;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * initializes an iterator object on the stack
 */
as_iterator * as_iterator_init(as_iterator * i, void * source, const as_iterator_hooks * hooks);

/**
 * Creates a new iterator for a given source and hooks.
 * this is done with MALLOC and thus the iterator must be freed exactly once
 *
 * @param source the source feeding the iterator
 * @param hooks the hooks that interface with the source
 */
as_iterator * as_iterator_new(void * source, const as_iterator_hooks * hooks);

/**
 * Destroys the iterator, first by calling the hooks->destroy(), then freeing the memory
 * used by the iterator.
 */
void as_iterator_destroy(as_iterator * i);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Tests if there are more values available in the iterator.
 *
 * @param i the iterator to be tested.
 * @return true if there are more values, otherwise false.
 */
inline bool as_iterator_has_next(const as_iterator * i) {
  return as_util_hook(has_next, false, i);
}

/**
 * Attempts to get the next value from the iterator.
 * This will return the next value, and iterate past the value.
 *
 * @param i the iterator to get the next value from.
 * @return the next value available in the iterator.
 */
inline const as_val * as_iterator_next(as_iterator * i) {
  return as_util_hook(next, NULL, i);
}
