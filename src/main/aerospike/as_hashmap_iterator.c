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

#include <stdlib.h>

#include <citrusleaf/cf_shash.h>
#include <citrusleaf/alloc.h>

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_pair.h>

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static bool     as_hashmap_iterator_has_next(const as_iterator * i);
static as_val * as_hashmap_iterator_next(as_iterator * i);
static void     as_hashmap_iterator_destroy(as_iterator * i);

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

const as_iterator_hooks as_hashmap_iterator_hooks = {
    .destroy    = as_hashmap_iterator_destroy,
    .has_next   = as_hashmap_iterator_has_next,
    .next       = as_hashmap_iterator_next
};

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_iterator * as_hashmap_iterator_new(const as_hashmap * m)
{
    as_iterator * i = (as_iterator *) malloc(sizeof(as_iterator));
    if ( !i ) return i;

    i->free = true;
    i->hooks = &as_hashmap_iterator_hooks;
    i->data.hashmap.htable = m->htable;
    i->data.hashmap.curr = NULL;
    i->data.hashmap.next = NULL;
    i->data.hashmap.size = (uint32_t) ((shash *) m->htable)->table_len;
    i->data.hashmap.pos = 0;
    return i;
}

as_iterator * as_hashmap_iterator_init(const as_hashmap * m, as_iterator * i)
{
    if ( !i ) return i;

    i->free = false;
    i->hooks = &as_hashmap_iterator_hooks;
    i->data.hashmap.htable = m->htable;
    i->data.hashmap.curr = NULL;
    i->data.hashmap.next = NULL;
    i->data.hashmap.size = (uint32_t) ((shash *) m->htable)->table_len;
    i->data.hashmap.pos = 0;
    return i;
}

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static bool as_hashmap_iterator_seek(as_hashmap_iterator * it)
{
	shash * htable = (shash *) it->htable;
	shash_elem * curr = (shash_elem *) it->curr;
	shash_elem * next = (shash_elem *) it->next;
	uint32_t pos = it->pos;
	uint32_t size = it->size;

    // We no longer have slots in the table
    if ( pos > size ) return false;

    // If curr is set, that means we have a value ready to be read.
    if ( curr != NULL ) return true;

    // If next is set, that means we have something to iterate to.
    if ( next != NULL ) {
        if ( next->in_use ) {
            curr = next;
            next = curr->next;

            if ( !next ) {
                pos++;
            }

            it->curr = curr;
            it->next = next;
            it->pos = pos;
            return true;
        }
        else {
            pos++;
            next = NULL;
        }
    }

    // Iterate over the slots in the table
    for( ; pos < size; pos++ ) {

        // Get the bucket in the current slot
        curr = (shash_elem *) (((byte *) htable->table) + (SHASH_ELEM_SZ(htable) * pos));
        
        // If the bucket has a value, then return true
        if ( curr && curr->in_use ) {
            
            // we set next, so we have the next item in the bucket
            next = curr->next;

            // if next is empty, then we will move to the next bucket
            if ( !next ) pos++;

            it->curr = curr;
            it->next = next;
            it->pos = pos;
            return true;
        }
        else {
            curr = NULL;
            next = NULL;
        }
    }
    
    curr = NULL;
    next = NULL;
    pos = it->size;
    
    it->curr = curr;
    it->next = next;
    it->pos = pos;
    
    return false;
}

static void as_hashmap_iterator_destroy(as_iterator * i)
{
    return;
}

static bool as_hashmap_iterator_has_next(const as_iterator * i)
{
    as_hashmap_iterator * it = (as_hashmap_iterator *) &i->data.hashmap;
    return as_hashmap_iterator_seek(it);
}

static as_val * as_hashmap_iterator_next(as_iterator * i)
{
    as_hashmap_iterator * it = (as_hashmap_iterator *) &i->data.hashmap;

    if ( !as_hashmap_iterator_seek(it) ) return NULL;

    shash *         h   = it->htable;
    shash_elem *    e   = it->curr;
    as_pair **      p   = (as_pair **) SHASH_ELEM_VALUE_PTR(h, e);
    
    it->curr = NULL; // consume the value, so we can get the next one.
    
    return (as_val *) *p;
}
