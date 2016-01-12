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
#pragma once

#include <aerospike/as_serializer.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	const unsigned char * buffer;
	int offset;
	int length;
} as_unpacker;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_serializer * as_msgpack_new();
as_serializer * as_msgpack_init(as_serializer *);

int as_pack_val(as_packer * pk, const as_val * val);
int as_unpack_val(as_unpacker * pk, as_val ** val);

/******************************************************************************
 * Pack direct functions
 ******************************************************************************/

/**
 * Pack a list header with ele_count.
 * @return 0 on success
 */
int as_pack_list_header(as_packer *pk, uint32_t ele_count);
/**
 * Get packed header size for list with ele_count.
 * @return header size in bytes
 */
uint32_t as_pack_list_header_get_size(uint32_t ele_count);

/******************************************************************************
 * Unpack direct functions
 ******************************************************************************/

as_val_t as_unpack_peek_type(const as_unpacker *pk);
as_val_t as_unpack_buf_peek_type(const uint8_t *buf, uint32_t size);
/**
 * Get size of packed value.
 * @return negative int on error, size on success
 */
int64_t as_unpack_size(as_unpacker *pk);
/**
 * Get size of packed blob.
 * @return negative int on error, size on success
 */
int64_t as_unpack_blob_size(as_unpacker *pk);
/**
 * Unpack integer.
 * @return 0 if success
 */
int as_unpack_int64(as_unpacker *pk, int64_t *i);
int as_unpack_uint64(as_unpacker *pk, uint64_t *i);
/**
 * Unpack double.
 * @return 0 if success
 */
int as_unpack_double(as_unpacker *pk, double *x);
/**
 * Unpack list element count from buffer.
 */
int64_t as_unpack_buf_list_element_count(const uint8_t *buf, uint32_t size);
/**
 * Get element count of packed list.
 * @return negative int on failure, element count on success
 */
int64_t as_unpack_list_header_element_count(as_unpacker *pk);

#ifdef __cplusplus
} // end extern "C"
#endif
