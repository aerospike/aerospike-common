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
#include <aerospike/as_aerospike.h>
#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_clock.h>
#include <stdlib.h>

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
