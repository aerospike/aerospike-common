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
#include <aerospike/as_serializer.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_serializer * as_serializer_cons(as_serializer * serializer, bool free, void * data, const as_serializer_hooks * hooks) 
{
	if ( !serializer ) return serializer;

    serializer->free = false;
    serializer->data = data;
    serializer->hooks = hooks;
    return serializer;
}

as_serializer * as_serializer_init(as_serializer * serializer, void * data, const as_serializer_hooks * hooks) 
{
    return as_serializer_cons(serializer, false, data, hooks);
}

as_serializer * as_serializer_new(void * data, const as_serializer_hooks * hooks)
{
    as_serializer * serializer = (as_serializer *) cf_malloc(sizeof(as_serializer));
    return as_serializer_cons(serializer, true, data, hooks);
}

void as_serializer_destroy(as_serializer * serializer)
{
    if ( serializer->free ) {
    	cf_free(serializer);
    }
}


