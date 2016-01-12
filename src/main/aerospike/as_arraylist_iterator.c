/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <citrusleaf/alloc.h>

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_iterator.h>

#include <stdbool.h>
#include <stdlib.h>

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_iterator_hooks as_arraylist_iterator_hooks;

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

as_arraylist_iterator * as_arraylist_iterator_init(as_arraylist_iterator * iterator, const as_arraylist * list)
{
	if ( !iterator ) return iterator;

	as_iterator_init((as_iterator *) iterator, false, NULL, &as_arraylist_iterator_hooks);
	iterator->list = list;
	iterator->pos = 0;
	return iterator;
}

as_arraylist_iterator * as_arraylist_iterator_new(const as_arraylist * list)
{
	as_arraylist_iterator * iterator = (as_arraylist_iterator *) cf_malloc(sizeof(as_arraylist_iterator));
	if ( !iterator ) return iterator;

	as_iterator_init((as_iterator *) iterator, true, NULL, &as_arraylist_iterator_hooks);
	iterator->list = list;
	iterator->pos = 0;
	return iterator;
}

bool as_arraylist_iterator_release(as_arraylist_iterator * iterator) 
{
	iterator->list = NULL;
	iterator->pos = 0;
	return true;
}

void as_arraylist_iterator_destroy(as_arraylist_iterator * iterator) 
{
	as_iterator_destroy((as_iterator *) iterator);
}

bool as_arraylist_iterator_has_next(const as_arraylist_iterator * iterator) 
{
	return iterator && iterator->pos < iterator->list->size;
}

const as_val * as_arraylist_iterator_next(as_arraylist_iterator * iterator) 
{
	if ( iterator->pos < iterator->list->size ) {
		as_val * val = *(iterator->list->elements + iterator->pos);
		iterator->pos++;
		return val;
	}
	return NULL;
}
