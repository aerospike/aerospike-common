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

#include <stdbool.h>
#include <stdlib.h>

#include <citrusleaf/cf_alloc.h>

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_iterator.h>

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void     as_arraylist_iterator_destroy(as_iterator *);
static bool     as_arraylist_iterator_has_next(const as_iterator *);
static as_val * as_arraylist_iterator_next(as_iterator *);

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

const as_iterator_hooks as_arraylist_iterator_hooks = {
    .destroy    = as_arraylist_iterator_destroy,
    .has_next   = as_arraylist_iterator_has_next,
    .next       = as_arraylist_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_iterator * as_arraylist_iterator_new(const as_arraylist * l) {
    as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
    i->is_malloc = true;
    i->hooks = &as_arraylist_iterator_hooks;
    i->data.arraylist.list = l;
    i->data.arraylist.pos = 0;
    return i;
}

as_iterator * as_arraylist_iterator_init(const as_arraylist * l, as_iterator * i) {
    i->is_malloc = false;
    i->hooks = &as_arraylist_iterator_hooks;
    i->data.arraylist.list = l;
    i->data.arraylist.pos = 0;
    return i;
}

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void as_arraylist_iterator_destroy(as_iterator * i) {
    return;
}

static bool as_arraylist_iterator_has_next(const as_iterator * i) {
    const as_arraylist_iterator * it = &i->data.arraylist;
    return it->pos < it->list->size;
}

static as_val * as_arraylist_iterator_next(as_iterator * i) {
    as_arraylist_iterator * it = &i->data.arraylist;
    if ( it->pos < it->list->size ) {
        as_val * val = *(it->list->elements + it->pos);
        it->pos++;
        return val;
    }
    return NULL;
}

