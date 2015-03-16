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

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_clock.h>
#include <aerospike/as_aerospike.h>

#include "internal.h"


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_aerospike_rec_create(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_update(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_exists(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_remove(const as_aerospike * a, const as_rec * r);

extern inline int as_aerospike_log(const as_aerospike * a, const char * name, const int line, const int lvl, const char * msg);
extern inline cf_clock as_aerospike_get_current_time(const as_aerospike * a );
extern inline int as_aerospike_set_context(const as_aerospike *a, const as_rec *r, const uint32_t context);
extern inline int as_aerospike_get_config(const as_aerospike *a, const as_rec *r, const char *);

extern inline as_rec * as_aerospike_crec_create(const as_aerospike * a, const as_rec * r);
extern inline as_rec * as_aerospike_crec_open(const as_aerospike * a, const as_rec *r, const char *);
extern inline int as_aerospike_crec_remove(const as_aerospike * a, const as_rec * cr);
extern inline int as_aerospike_crec_update(const as_aerospike * a, const as_rec *cr);
extern inline int as_aerospike_crec_close(const as_aerospike * a, const as_rec *cr);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_init(as_aerospike * a, void * s, const as_aerospike_hooks * h)
{
	a->is_rcalloc = false;
	a->source = s;
	a->hooks = h;
	return a;
}

as_aerospike * as_aerospike_new(void * s, const as_aerospike_hooks * h)
{
	as_aerospike * a = (as_aerospike *) cf_rc_alloc(sizeof(as_aerospike));
	a->is_rcalloc = true;
	a->source = s;
	a->hooks = h;
	return a;
}

void as_aerospike_destroy(as_aerospike * a)
{
	if (a->is_rcalloc)
	   cf_rc_releaseandfree(a);
}

