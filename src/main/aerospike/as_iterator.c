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

#include <aerospike/as_iterator.h>
#include <aerospike/as_util.h>
#include <aerospike/as_val.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline bool as_iterator_has_next(const as_iterator * iterator);
extern inline const as_val * as_iterator_next(as_iterator * iterator);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_iterator * as_iterator_init(as_iterator * iterator, bool free, void * data, const as_iterator_hooks * hooks)
{
	if ( !iterator ) return iterator;

	iterator->free = free;
	iterator->data = data;
	iterator->hooks = hooks;
	return iterator;
}

void as_iterator_destroy(as_iterator * iterator)
{
    as_util_hook(destroy, 1, iterator);
	iterator->data = NULL;
	iterator->hooks = NULL;
    if ( iterator->free ) {
    	free(iterator);
    }
}

