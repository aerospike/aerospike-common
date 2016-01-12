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

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_list.h>
#include <aerospike/as_list_iterator.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*******************************************************************************
 *	EXTERN FUNCTIONS
 ******************************************************************************/

extern bool as_arraylist_release(as_arraylist * list);

/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

static bool _as_arraylist_list_destroy(as_list * l) 
{
	return as_arraylist_release((as_arraylist *) l);
}

/*******************************************************************************
 *	VALUE FUNCTIONS
 ******************************************************************************/

static uint32_t _as_arraylist_list_hashcode(const as_list * l) 
{
	return as_arraylist_hashcode((as_arraylist *) l);
}

static uint32_t _as_arraylist_list_size(const as_list * l) 
{
	return as_arraylist_size((as_arraylist *) l);
}

/*******************************************************************************
 *	GET FUNCTIONS
 ******************************************************************************/

static as_val * _as_arraylist_list_get(const as_list * l, uint32_t i)
{
	return as_arraylist_get((as_arraylist *) l, i);
}

static int64_t _as_arraylist_list_get_int64(const as_list * l, uint32_t i)
{
	return as_arraylist_get_int64((as_arraylist *) l, i);
}

static double _as_arraylist_list_get_double(const as_list * l, uint32_t i)
{
	return as_arraylist_get_double((as_arraylist *) l, i);
}

static char * _as_arraylist_list_get_str(const as_list * l, uint32_t i)
{
	return as_arraylist_get_str((as_arraylist *) l, i);
}

/*******************************************************************************
 *	SET FUNCTIONS
 ******************************************************************************/

static int _as_arraylist_list_set(as_list * l, uint32_t i, as_val * v)
{
	return as_arraylist_set((as_arraylist *) l, i, v);
}

static int _as_arraylist_list_set_int64(as_list * l, uint32_t i, int64_t v)
{
	return as_arraylist_set_int64((as_arraylist *) l, i, v);
}

static int _as_arraylist_list_set_double(as_list * l, uint32_t i, double v)
{
	return as_arraylist_set_double((as_arraylist *) l, i, v);
}

static int _as_arraylist_list_set_str(as_list * l, uint32_t i, const char * v)
{
	return as_arraylist_set_str((as_arraylist *) l, i, v);
}

/*******************************************************************************
 *	INSERT FUNCTIONS
 ******************************************************************************/

static int _as_arraylist_list_insert(as_list * l, uint32_t i, as_val * v)
{
	return as_arraylist_insert((as_arraylist *) l, i, v);
}

static int _as_arraylist_list_insert_int64(as_list * l, uint32_t i, int64_t v)
{
	return as_arraylist_insert_int64((as_arraylist *) l, i, v);
}

static int _as_arraylist_list_insert_double(as_list * l, uint32_t i, double v)
{
	return as_arraylist_insert_double((as_arraylist *) l, i, v);
}

static int _as_arraylist_list_insert_str(as_list * l, uint32_t i, const char * v)
{
	return as_arraylist_insert_str((as_arraylist *) l, i, v);
}

/*******************************************************************************
 *	APPEND FUNCTIONS
 ******************************************************************************/

static int _as_arraylist_list_append(as_list * l, as_val * v) 
{
	return as_arraylist_append((as_arraylist *) l, v);
}

static int _as_arraylist_list_append_int64(as_list * l, int64_t v) 
{
	return as_arraylist_append_int64((as_arraylist *) l, v);
}

static int _as_arraylist_list_append_double(as_list * l, double v)
{
	return as_arraylist_append_double((as_arraylist *) l, v);
}

static int _as_arraylist_list_append_str(as_list * l, const char * v)
{
	return as_arraylist_append_str((as_arraylist *) l, v);
}

/*******************************************************************************
 *	PREPEND FUNCTIONS
 ******************************************************************************/

static int _as_arraylist_list_prepend(as_list * l, as_val * v) 
{
	return as_arraylist_prepend((as_arraylist *) l, v);
}

static int _as_arraylist_list_prepend_int64(as_list * l, int64_t v) 
{
	return as_arraylist_prepend_int64((as_arraylist *) l, v);
}

static int _as_arraylist_list_prepend_double(as_list * l, double v)
{
	return as_arraylist_prepend_double((as_arraylist *) l, v);
}

static int _as_arraylist_list_prepend_str(as_list * l, const char * v)
{
	return as_arraylist_prepend_str((as_arraylist *) l, v);
}

/*******************************************************************************
 *	REMOVE FUNCTION
 ******************************************************************************/

static int _as_arraylist_list_remove(as_list * l, uint32_t i)
{
	return as_arraylist_remove((as_arraylist *) l, i);
}

/*******************************************************************************
 *	ACCESSOR AND MODIFIER FUNCTIONS
 ******************************************************************************/

static int _as_arraylist_list_concat(as_list * l, const as_list * l2)
{
	return as_arraylist_concat((as_arraylist *) l, (const as_arraylist *) l2);
}

static int _as_arraylist_list_trim(as_list * l, uint32_t i)
{
	return as_arraylist_trim((as_arraylist *) l, i);
}

static as_val * _as_arraylist_list_head(const as_list * l) 
{
	return as_arraylist_head((as_arraylist *) l);
}

static as_list * _as_arraylist_list_tail(const as_list * l) 
{
	return (as_list *) as_arraylist_tail((as_arraylist *) l);
}

static as_list * _as_arraylist_list_drop(const as_list * l, uint32_t n) 
{
	return (as_list *) as_arraylist_drop((as_arraylist *) l, n);
}

static as_list * _as_arraylist_list_take(const as_list * l, uint32_t n) 
{
	return (as_list *) as_arraylist_take((as_arraylist *) l, n);
}

/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

static bool _as_arraylist_list_foreach(const as_list * l, as_list_foreach_callback callback, void * udata) 
{
	return as_arraylist_foreach((as_arraylist *) l, callback, udata);
}

static as_list_iterator * _as_arraylist_list_iterator_new(const as_list * l) 
{
	return (as_list_iterator *) as_arraylist_iterator_new((as_arraylist *) l);
}

static as_list_iterator * _as_arraylist_list_iterator_init(const as_list * l, as_list_iterator * it) 
{
	return (as_list_iterator *) as_arraylist_iterator_init((as_arraylist_iterator *) it, (as_arraylist *) l);
}

/*******************************************************************************
 *	HOOKS
 ******************************************************************************/

const as_list_hooks as_arraylist_list_hooks = {

	/***************************************************************************
	 *	instance hooks
	 **************************************************************************/

	.destroy	= _as_arraylist_list_destroy,

	/***************************************************************************
	 *	info hooks
	 **************************************************************************/

	.hashcode	= _as_arraylist_list_hashcode,
	.size		= _as_arraylist_list_size,

	/***************************************************************************
	 *	get hooks
	 **************************************************************************/

	.get		= _as_arraylist_list_get,
	.get_int64	= _as_arraylist_list_get_int64,
	.get_double = _as_arraylist_list_get_double,
	.get_str	= _as_arraylist_list_get_str,

	/***************************************************************************
	 *	set hooks
	 **************************************************************************/

	.set		= _as_arraylist_list_set,
	.set_int64	= _as_arraylist_list_set_int64,
	.set_double	= _as_arraylist_list_set_double,
	.set_str	= _as_arraylist_list_set_str,

	/***************************************************************************
	 *	insert hooks
	 **************************************************************************/

	.insert			= _as_arraylist_list_insert,
	.insert_int64	= _as_arraylist_list_insert_int64,
	.insert_double	= _as_arraylist_list_insert_double,
	.insert_str		= _as_arraylist_list_insert_str,

	/***************************************************************************
	 *	append hooks
	 **************************************************************************/

	.append			= _as_arraylist_list_append,
	.append_int64	= _as_arraylist_list_append_int64,
	.append_double	= _as_arraylist_list_append_double,
	.append_str		= _as_arraylist_list_append_str,

	/***************************************************************************
	 *	prepend hooks
	 **************************************************************************/

	.prepend		= _as_arraylist_list_prepend,
	.prepend_int64	= _as_arraylist_list_prepend_int64,
	.prepend_double	= _as_arraylist_list_prepend_double,
	.prepend_str	= _as_arraylist_list_prepend_str,

	/***************************************************************************
	 *	remove hook
	 **************************************************************************/

	.remove		= _as_arraylist_list_remove,

	/***************************************************************************
	 *	accessor and modifier hooks
	 **************************************************************************/

	.concat		= _as_arraylist_list_concat,
	.trim		= _as_arraylist_list_trim,
	.head		= _as_arraylist_list_head,
	.tail		= _as_arraylist_list_tail,
	.drop		= _as_arraylist_list_drop,
	.take		= _as_arraylist_list_take,

	/***************************************************************************
	 *	iteration hooks
	 **************************************************************************/

	.foreach		= _as_arraylist_list_foreach,
	.iterator_new	= _as_arraylist_list_iterator_new,
	.iterator_init	= _as_arraylist_list_iterator_init,

};
