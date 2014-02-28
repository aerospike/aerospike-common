/******************************************************************************
 *	Copyright 2008-2013 by Aerospike.
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

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_shash.h>

#include <aerospike/as_hashmap.h>
#include <aerospike/as_hashmap_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_pair.h>

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_iterator_hooks as_hashmap_iterator_hooks;

/******************************************************************************
 *	STATIC FUNCTIONS
 *****************************************************************************/

static bool as_hashmap_iterator_seek(as_hashmap_iterator * it)
>>>>>>> ee8a19a8ecdc32692467512ce95b0ebc6839a843
{
	// We no longer have slots in the table
	if ( it->pos > it->size ) return false;

	// If curr is set, that means we have a value ready to be read.
	if ( it->curr != NULL ) return true;

	// If next is set, that means we have something to iterate to.
	if ( it->next != NULL ) {
		if ( ((shash_elem *) it->next)->in_use ) {
			it->curr = it->next;
			it->next = ((shash_elem *) it->curr)->next;
			if ( !it->next ) {
				it->pos++;
			}
			return true;
		}
		else {
			it->pos++;
			it->next = NULL;
		}
	}

	// Iterate over the slots in the table
	for( ; it->pos < it->size; it->pos++ ) {

		// Get the bucket in the current slot
		it->curr = (shash_elem *) (((byte *) ((shash *) it->htable)->table) + (SHASH_ELEM_SZ(((shash *) it->htable)) * it->pos));

		// If the bucket has a value, then return true
		if ( it->curr && ((shash_elem *) it->curr)->in_use ) {
			
			// we set next, so we have the next item in the bucket
			it->next = ((shash_elem *) it->curr)->next;

			// if next is empty, then we will move to the next bucket
			if ( !it->next ) it->pos++;

			return true;
		}
		else {
			it->curr = NULL;
			it->next = NULL;
		}
	}
	
	it->curr = NULL;
	it->next = NULL;
	it->pos = it->size;
	return false;
}

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

as_hashmap_iterator * as_hashmap_iterator_init(as_hashmap_iterator * iterator, const as_hashmap * map)
{
	if ( !iterator ) return iterator;

	as_iterator_init((as_iterator *) iterator, false, NULL, &as_hashmap_iterator_hooks);
	iterator->htable = map->htable;
	iterator->curr = NULL;
	iterator->next = NULL;
	iterator->size = (uint32_t) ((shash *) map->htable)->table_len;
	iterator->pos = 0;
	return iterator;
}

as_hashmap_iterator * as_hashmap_iterator_new(const as_hashmap * map)
{
	as_hashmap_iterator * iterator = (as_hashmap_iterator *) cf_malloc(sizeof(as_hashmap_iterator));
	if ( !iterator ) return iterator;

	as_iterator_init((as_iterator *) iterator, true, NULL, &as_hashmap_iterator_hooks);
	iterator->htable = map->htable;
	iterator->curr = NULL;
	iterator->next = NULL;
	iterator->size = (uint32_t) ((shash *) map->htable)->table_len;
	iterator->pos = 0;
	return iterator;
}

bool as_hashmap_iterator_release(as_hashmap_iterator * iterator)
{
	iterator->htable = NULL;
	iterator->curr = NULL;
	iterator->next = NULL;
	iterator->size = 0;
	iterator->pos = 0;
	return true;
}

void as_hashmap_iterator_destroy(as_hashmap_iterator * iterator)
{
	as_iterator_destroy((as_iterator *) iterator);
}

bool as_hashmap_iterator_has_next(const as_hashmap_iterator * iterator)
{
	return as_hashmap_iterator_seek((as_hashmap_iterator *) iterator);
}

const as_val * as_hashmap_iterator_next(as_hashmap_iterator * iterator)
{
	if ( !as_hashmap_iterator_seek(iterator) ) return NULL;

	shash *         h   = iterator->htable;
	shash_elem *    e   = iterator->curr;
	as_pair **      p   = (as_pair **) SHASH_ELEM_VALUE_PTR(h, e);
	
	iterator->curr = NULL; // consume the value, so we can get the next one.
	
	return (as_val *) *p;
}
