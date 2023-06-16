/* 
 * Copyright 2008-2023 Aerospike, Inc.
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
#include <aerospike/as_map.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_map_iterator.h>
#include <aerospike/as_pair.h>
#include <citrusleaf/alloc.h>
#include <string.h>

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_map* as_map_cons(as_map* map, bool free, uint32_t flags, const as_map_hooks* hooks)
{
	if ( !map ) return map;

	as_val_cons((as_val*) map, AS_MAP, free);
	map->flags = flags;
	map->hooks = hooks;
	return map;
}

as_map* as_map_init(as_map* map, const as_map_hooks* hooks)
{
	return as_map_cons(map, false, 0, hooks);
}

as_map* as_map_new(const as_map_hooks* hooks)
{
	as_map* map = (as_map*) cf_malloc(sizeof(as_map));
	return as_map_cons(map, true, 0, hooks);
}

/******************************************************************************
 * as_val FUNCTIONS
 ******************************************************************************/

void as_map_val_destroy(as_val* v) {
	as_map* m = as_map_fromval(v);
	as_util_hook(destroy, false, m);
}

uint32_t as_map_val_hashcode(const as_val* v) {
	as_map* m = as_map_fromval(v);
	return as_util_hook(hashcode, 0, m);
}

typedef struct as_map_val_tostring_data_s {
	char *		buf;
	uint32_t	blk;
	uint32_t	cap;
	uint32_t	pos;
	bool		sep;
} as_map_val_tostring_data;

static bool as_map_val_tostring_foreach(const as_val* key, const as_val* val, void* udata)
{
	as_map_val_tostring_data * data = (as_map_val_tostring_data *) udata;

	char * keystr = as_val_tostring(key);
	if (!keystr) {
		return false;
	}
	int keylen = (int)strlen(keystr);

	char * valstr = as_val_tostring(val);
    if (!valstr) {
		cf_free(keystr);
    	return false;
    }
	int vallen = (int)strlen(valstr);

	uint32_t entlen = keylen + 2 + vallen + 2;

	if ( data->pos + entlen >= data->cap ) {
		uint32_t adj = entlen > data->blk ? entlen : data->blk;
		data->buf = cf_realloc(data->buf, sizeof(char) * (data->cap + adj));
		memset(data->buf + data->cap, 0, sizeof(char) * adj);
		data->cap += adj;
	}

	if ( data->sep ) {
		data->buf[data->pos] = ',';
		data->buf[data->pos + 1] = ' ';
		data->pos += 2;
	}

	strcpy(data->buf + data->pos, keystr);
	data->pos += keylen;

	strcpy(data->buf + data->pos, ":");
	data->pos += 1;

	strcpy(data->buf + data->pos, valstr);
	data->pos += vallen;

	data->sep = true;

	cf_free(keystr);
	keystr = NULL;

	cf_free(valstr);
	valstr = NULL;

	return true;
}

char * as_map_val_tostring(const as_val* v)
{
	as_map_val_tostring_data data = {
		.buf = NULL,
		.blk = 1024,
		.cap = 1024,
		.pos = 0,
		.sep = false
	};

	data.buf = (char *) cf_calloc(data.cap, sizeof(char));

	strcpy(data.buf, "{");
	data.pos += 1;
	
	if ( v ) {
		as_map_foreach((as_map*) v, as_map_val_tostring_foreach, &data);
	}

	if ( data.pos + 2 >= data.cap ) {
		data.buf = cf_realloc(data.buf, sizeof(char) * (data.cap + 2));
		data.cap += 2;
	}

	data.buf[data.pos] = '}';
	data.buf[data.pos + 1] = 0;
	
	return data.buf;
}
