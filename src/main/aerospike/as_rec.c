/* 
 * Copyright 2008-2014 Aerospike, Inc.
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

#include <stdlib.h>
#include <stdio.h>

#include <citrusleaf/alloc.h>

#include <aerospike/as_rec.h>
#include <aerospike/as_bytes.h>

/******************************************************************************
 *	INLINES
 ******************************************************************************/

extern inline void			as_rec_destroy(as_rec * rec);

extern inline void *		as_rec_source(const as_rec * rec);

extern inline int			as_rec_remove(const as_rec * rec, const char * name);
extern inline uint32_t		as_rec_ttl(const as_rec * rec);
extern inline uint16_t		as_rec_gen(const as_rec * rec);
extern inline as_bytes *	as_rec_digest(const as_rec * rec);
extern inline uint16_t		as_rec_numbins(const as_rec * rec);
extern inline int			as_rec_bin_names(const as_rec * rec, as_rec_bin_names_callback callback, void * udata);
extern inline int			as_rec_set_flags(const as_rec * rec, const char * name, uint8_t flags);
extern inline int			as_rec_set_type(const as_rec * rec, int8_t rec_type);
extern inline int			as_rec_set_ttl(const as_rec * rec, uint32_t ttl);
extern inline int			as_rec_drop_key(const as_rec * rec);

extern inline as_val *		as_rec_get(const as_rec * rec, const char * name);
extern inline int64_t		as_rec_get_int64(const as_rec * rec, const char * name);
extern inline char *		as_rec_get_str(const as_rec * rec, const char * name);
extern inline as_integer *	as_rec_get_integer(const as_rec * rec, const char * name);
extern inline as_string *	as_rec_get_string(const as_rec * rec, const char * name);
extern inline as_bytes *	as_rec_get_bytes(const as_rec * rec, const char * name);
extern inline as_list *		as_rec_get_list(const as_rec * rec, const char * name);
extern inline as_map *		as_rec_get_map(const as_rec * rec, const char * name);

extern inline int			as_rec_set(const as_rec * rec, const char * name, const as_val * value);
extern inline int			as_rec_set_int64(const as_rec * rec, const char * name, int64_t value);
extern inline int			as_rec_set_str(const as_rec * rec, const char * name, const char * value);
extern inline int			as_rec_set_integer(const as_rec * rec, const char * name, const as_integer * value);
extern inline int			as_rec_set_string(const as_rec * rec, const char * name, const as_string * value);
extern inline int			as_rec_set_bytes(const as_rec * rec, const char * name, const as_bytes * value);
extern inline int			as_rec_set_list(const as_rec * rec, const char * name, const as_list * value);
extern inline int			as_rec_set_map(const as_rec * rec, const char * name, const as_map * value);

extern inline bool 			as_rec_foreach(const as_rec * rec, as_rec_foreach_callback callback, void * udata);

extern inline as_val *		as_rec_toval(const as_rec * rec);
extern inline as_rec *		as_rec_fromval(const as_val * v);

/******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

as_rec * as_rec_cons(as_rec * rec, bool free, void * data, const as_rec_hooks * hooks) 
{
	if ( !rec ) return rec;
	
	as_val_init((as_val *) rec, AS_REC, free);
	rec->data = data;
	rec->hooks = hooks;
	return rec;
}

as_rec * as_rec_init(as_rec * rec, void * data, const as_rec_hooks * hooks) 
{
	return as_rec_cons(rec, false, data, hooks);
}

as_rec * as_rec_new(void * data, const as_rec_hooks * hooks) 
{
	as_rec * rec = (as_rec *) cf_malloc(sizeof(as_rec));
	return as_rec_cons(rec, true, data, hooks);
}

/******************************************************************************
 *	as_val FUNCTIONS
 ******************************************************************************/

void as_rec_val_destroy(as_val *v)
{
	as_rec * rec = as_rec_fromval(v);
	as_util_hook(destroy, false, rec);
}

uint32_t as_rec_val_hashcode(const as_val * v)
{
	as_rec * rec = as_rec_fromval(v);
	return as_util_hook(hashcode, 0, rec);
}

char * as_rec_val_tostring(const as_val * v)
{
	return cf_strdup("[ REC ]");
}
