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

#include <aerospike/as_iterator.h>
#include <aerospike/as_list.h>
#include <aerospike/as_list_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_util.h>

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "internal.h"

/******************************************************************************
 *	INLINE FUNCTIONS
 *****************************************************************************/

extern inline void			as_list_destroy(as_list * l);

extern inline uint32_t		as_list_size(as_list * l);
extern inline as_val *		as_list_head(const as_list * l);
extern inline as_list *		as_list_tail(const as_list * l);
extern inline as_list *		as_list_drop(const as_list * l, uint32_t n);
extern inline as_list *		as_list_take(const as_list * l, uint32_t n);

extern inline int 			as_list_append(as_list * l, as_val * v);
extern inline int 			as_list_append_int64(as_list * l, int64_t v);
extern inline int 			as_list_append_str(as_list * l, const char * v);
extern inline int 			as_list_append_integer(as_list * l, as_integer * v);
extern inline int 			as_list_append_string(as_list * l, as_string * v);
extern inline int 			as_list_append_bytes(as_list * l, as_bytes * v);
extern inline int 			as_list_append_list(as_list * l, as_list * v);
extern inline int 			as_list_append_map(as_list * l, as_map * v);

extern inline int 			as_list_prepend(as_list * l, as_val * v);
extern inline int 			as_list_prepend_int64(as_list * l, int64_t v);
extern inline int 			as_list_prepend_str(as_list * l, const char * v);
extern inline int 			as_list_prepend_integer(as_list * l, as_integer * v);
extern inline int 			as_list_prepend_string(as_list * l, as_string * v);
extern inline int 			as_list_prepend_bytes(as_list * l, as_bytes * v);
extern inline int 			as_list_prepend_list(as_list * l, as_list * v);
extern inline int 			as_list_prepend_map(as_list * l, as_map * v);

extern inline as_val *		as_list_get(const as_list * l, const uint32_t i);
extern inline int64_t 		as_list_get_int64(const as_list * l, const uint32_t i);
extern inline char *		as_list_get_str(const as_list * l, const uint32_t i);
extern inline as_integer *	as_list_get_integer(const as_list * l, const uint32_t i);
extern inline as_string *	as_list_get_string(const as_list * l, const uint32_t i);
extern inline as_bytes *	as_list_get_bytes(const as_list * l, const uint32_t i);
extern inline as_list *		as_list_get_list(const as_list * l, const uint32_t i);
extern inline as_map *		as_list_get_map(const as_list * l, const uint32_t i);

extern inline int			as_list_set(as_list * l, const uint32_t i, as_val * v);
extern inline int			as_list_set_int64(as_list * l, const uint32_t i, int64_t v);
extern inline int			as_list_set_str(as_list * l, const uint32_t i, const char * v);
extern inline int			as_list_set_integer(as_list * l, const uint32_t i, as_integer * v);
extern inline int			as_list_set_string(as_list * l, const uint32_t i, as_string * v);
extern inline int			as_list_set_bytes(as_list * l, const uint32_t i, as_bytes * v);
extern inline int			as_list_set_list(as_list * l, const uint32_t i, as_list * v);
extern inline int			as_list_set_map(as_list * l, const uint32_t i, as_map * v);

extern inline bool					as_list_foreach(const as_list * l, as_list_foreach_callback callback, void * udata);
extern inline as_list_iterator *	as_list_iterator_new(const as_list * l);
extern inline as_list_iterator *	as_list_iterator_init(as_list_iterator * it, const as_list * l);

extern inline as_val *		as_list_toval(as_list * l);
extern inline as_list *		as_list_fromval(as_val * v);

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

/**
 *	Initialize an instance of as_list.
 *	This should only be called from subtypes of as_list during initialization.
 */
as_list * as_list_cons(as_list * list, bool free, void * data, const as_list_hooks * hooks) 
{
	if ( !list ) return list;

	as_val_cons((as_val *) list, AS_LIST, free);
	list->data = data;
	list->hooks = hooks;
	return list;
}
/**
 *	Initialize an instance of as_list.
 *	This should only be called from subtypes of as_list during initialization.
 */
as_list * as_list_init(as_list * list, void * data, const as_list_hooks * hooks) 
{
	return as_list_cons(list, false, data, hooks);
}

/**
 *	Initialize an instance of as_list.
 *	This should only be called from subtypes of as_list during initialization.
 */
as_list * as_list_new(void * data, const as_list_hooks * hooks) 
{
	as_list * list = (as_list *) malloc(sizeof(as_list));
	return as_list_cons(list, true, data, hooks);
}

/******************************************************************************
 *	as_val FUNCTIONS
 *****************************************************************************/

void as_list_val_destroy(as_val * v) 
{
	as_list * l = as_list_fromval((as_val *) v);
	as_util_hook(destroy, false, l);
}

uint32_t as_list_val_hashcode(const as_val * v) 
{
	as_list * l = as_list_fromval((as_val *) v);
	return as_util_hook(hashcode, 0, l);
}

typedef struct as_list_val_tostring_data_s {
	char *		buf;
	uint32_t	blk;
	uint32_t	cap;
	uint32_t	pos;
	bool		sep;
} as_list_val_tostring_data;

static bool as_list_val_tostring_foreach(as_val * val, void * udata)
{
	as_list_val_tostring_data * data = (as_list_val_tostring_data *) udata;

	char * str = as_val_tostring(val);
	size_t len = strlen(str);

	if ( data->pos + len + 2 >= data->cap ) {
		uint32_t adj = ((len+2) > data->blk) ? len+2 : data->blk;
		data->buf = realloc(data->buf, sizeof(char) * (data->cap + adj));
		data->cap += adj;
	}

	if ( data->sep ) {
		data->buf[data->pos] = ',';
		data->buf[data->pos + 1] = ' ';
		data->pos += 2;
	}

	memcpy(data->buf + data->pos, str, len);
	data->pos += len;
	data->sep = true;

	free(str);

	return true;
}

char * as_list_val_tostring(const as_val * v) 
{
	as_list_val_tostring_data data = {
		.buf = NULL,
		.blk = 256,
		.cap = 1024,
		.pos = 0,
		.sep = false
	};

	data.buf = (char *) calloc(data.cap, sizeof(char));

	strcpy(data.buf, "List(");
	data.pos += 5;

	if ( v ) {
		as_list_foreach((as_list *) v, as_list_val_tostring_foreach, &data);
	}

	if ( data.pos + 2 >= data.cap ) {
		data.buf = realloc(data.buf, sizeof(char) * (data.cap + 2));
		data.cap += 2;
	}

	data.buf[data.pos] = ')';
	data.buf[data.pos + 1] = 0;
	
	return data.buf;
}
