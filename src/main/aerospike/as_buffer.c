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
#include <aerospike/as_buffer.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_buffer_init(as_buffer * b) {
    b->capacity = 0;
    b->size = 0;
    b->data = NULL;
    return 0;
}

void as_buffer_destroy(as_buffer * b) {
	if ( b->data ) { 
		cf_free(b->data);
		b->data = 0; 
	}
    return;
} 
 
