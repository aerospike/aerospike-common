/* 
 * Copyright 2008-2015 Aerospike, Inc.
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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <aerospike/as_serializer.h>

#define AS_PACKER_BUFFER_SIZE 8192

typedef struct as_packer_buffer {
	struct as_packer_buffer * next;
	unsigned char * buffer;
	int length;
} as_packer_buffer;

typedef struct as_packer {
	struct as_packer_buffer * head;
	struct as_packer_buffer * tail;
	unsigned char * buffer;
	int offset;
	int capacity;
} as_packer;

typedef struct as_unpacker {
	unsigned char * buffer;
	int offset;
	int length;
} as_unpacker;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_serializer * as_msgpack_new();
as_serializer * as_msgpack_init(as_serializer *);

int as_pack_val(as_packer * pk, as_val * val);
int as_unpack_val(as_unpacker * pk, as_val ** val);

#ifdef __cplusplus
} // end extern "C"
#endif
