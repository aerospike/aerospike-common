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

#include <stdlib.h>
#include <stdio.h>

#include <citrusleaf/alloc.h>

#include <aerospike/as_rec.h>
#include <aerospike/as_bytes.h>

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
