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

#include <citrusleaf/alloc.h>

#include <aerospike/as_iterator.h>
#include <aerospike/as_map.h>
#include <aerospike/as_map_iterator.h>
#include <aerospike/as_pair.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

/******************************************************************************
 * INLINES
 ******************************************************************************/

extern inline void				as_map_destroy(as_map * m);

extern inline uint32_t			as_map_size(const as_map * m);
extern inline as_val *			as_map_get(const as_map * m, const as_val * k);
extern inline int				as_map_set(as_map * m, const as_val * k, const as_val * v);
extern inline int				as_map_clear(as_map * m);
extern inline int				as_map_remove(as_map * m, const as_val * k);

extern inline bool				as_map_foreach(const as_map * m, as_map_foreach_callback callback, void * udata);
extern inline as_map_iterator *	as_map_iterator_new(const as_map * m);
extern inline as_map_iterator *	as_map_iterator_init(as_map_iterator * it, const as_map * m);

extern inline as_val *			as_map_toval(const as_map * m) ;
extern inline as_map *			as_map_fromval(const as_val * v);

/******************************************************************************
 *	FUNCTIONS
 *****************************************************************************/

as_map * as_map_cons(as_map * map, bool free, void * data, const as_map_hooks * hooks) 
{
	if ( !map ) return map;

	as_val_cons((as_val *) map, AS_MAP, free);
	map->data = data;
	map->hooks = hooks;
	return map;
}

as_map * as_map_init(as_map * map, void * data, const as_map_hooks * hooks) 
{
	return as_map_cons(map, false, data, hooks);
}

as_map * as_map_new(void * data, const as_map_hooks * hooks) 
{
	as_map * map = (as_map *) cf_malloc(sizeof(as_map));
	return as_map_cons(map, true, data, hooks);
}

/******************************************************************************
 * as_val FUNCTIONS
 ******************************************************************************/

void as_map_val_destroy(as_val * v) {
	as_map * m = as_map_fromval(v);
	as_util_hook(destroy, false, m);
}

uint32_t as_map_val_hashcode(const as_val *v) {
	as_map * m = as_map_fromval(v);
	return as_util_hook(hashcode, 0, m);
}

typedef struct as_map_val_tostring_data_s {
	char *		buf;
	uint32_t	blk;
	uint32_t	cap;
	uint32_t	pos;
	bool		sep;
} as_map_val_tostring_data;

static bool as_map_val_tostring_foreach(const as_val * key, const as_val * val, void * udata)
{
	as_map_val_tostring_data * data = (as_map_val_tostring_data *) udata;

	char * keystr = as_val_tostring(key);
	int keylen = (int)strlen(keystr);
	if (!keystr) {
		return false;
	}

	char * valstr = as_val_tostring(val);
    if (!valstr) {
    	return false;
    }
	int vallen = (int)strlen(valstr);

	if ( data->sep ) {
		data->buf[data->pos] = ',';
		data->buf[data->pos + 1] = ' ';
		data->pos += 2;
	}

	int entlen = keylen + 2 + vallen + 2;

	if ( data->pos + entlen >= data->cap ) {
		uint32_t adj = entlen > data->blk ? entlen : data->blk;
		data->buf = cf_realloc(data->buf, sizeof(char) * (data->cap + adj));
		bzero(data->buf + data->cap, sizeof(char) * adj);
		data->cap += adj;
	}

	strncpy(data->buf + data->pos, keystr, keylen);
	data->pos += keylen;

	strcpy(data->buf + data->pos, ":");
	data->pos += 1;

	strncpy(data->buf + data->pos, valstr, vallen);
	data->pos += vallen;

	data->sep = true;

	cf_free(keystr);
	keystr = NULL;

	cf_free(valstr);
	valstr = NULL;

	return true;
}

char * as_map_val_tostring(const as_val * v)
{
	as_map_val_tostring_data data = {
		.buf = NULL,
		.blk = 256,
		.cap = 1024,
		.pos = 0,
		.sep = false
	};

	data.buf = (char *) cf_calloc(data.cap, sizeof(char));

	strcpy(data.buf, "{");
	data.pos += 1;
	
	if ( v ) {
		as_map_foreach((as_map *) v, as_map_val_tostring_foreach, &data);
	}

	if ( data.pos + 2 >= data.cap ) {
		data.buf = cf_realloc(data.buf, sizeof(char) * (data.cap + 2));
		data.cap += 2;
	}

	data.buf[data.pos] = '}';
	data.buf[data.pos + 1] = 0;
	
	return data.buf;
}
