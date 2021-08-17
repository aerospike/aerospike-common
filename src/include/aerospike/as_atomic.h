/* 
 * Copyright 2008-2018 Aerospike, Inc.
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

#if defined(_MSC_VER)
#include <aerospike/as_atomic_win.h>
#else
#include <aerospike/as_atomic_gcc.h>
#endif

/******************************************************************************
 * LOAD
 *****************************************************************************/

// double as_load_double(const double* target)
#define as_load_double(_target) ({ \
		uint64_t v =  as_load_uint64((const uint64_t*)_target); \
		*(double*)&v; \
	})

// float as_load_float(const float* target)
#define as_load_float(_target) ({ \
		uint32_t v =  as_load_uint32((const uint32_t*)_target); \
		*(float*)&v; \
	})

// Note - this assumes bool is implemented as a single byte.
// bool as_load_bool(const bool* target)
#define as_load_bool(_target) ({ \
		(bool)as_load_uint8((const uint8_t*)_target); \
	})

/******************************************************************************
 * STORE
 *****************************************************************************/

// void as_store_double(double* target, double value)
#define as_store_double(_target, _value) ({ \
		double v = _value; \
		as_store_uint64((uint64_t*)_target, *(uint64_t*)&v); \
	})

// void as_store_float(float* target, float value)
#define as_store_float(_target, _value) ({ \
		float v = _value; \
		as_store_uint32((uint32_t*)_target, *(uint32_t*)&v); \
	})

// Note - this assumes bool is implemented as a single byte.
// void as_store_bool(bool* target, bool value)
#define as_store_bool(_target, _value) ({ \
		as_store_uint8((uint8_t*)_target, (uint8_t)_value); \
	})

/******************************************************************************
 * FETCH AND SWAP
 *****************************************************************************/

// Note - this assumes pointers are 8 bytes.
// void* as_fas_ptr(void** target, void* value)
#define as_fas_ptr(_target, _value) ({ \
		(void*)as_fas_uint64((uint64_t*)_target, (uint64_t)_value); \
	})

// double as_fas_double(double* target, double value)
#define as_fas_double(_target, _value) ({ \
		double nv = _value; \
		uint64_t ov = as_fas_uint64((uint64_t*)_target, *(uint64_t*)&nv); \
		*(double*)&ov; \
	})

// float as_fas_float(float* target, float value)
#define as_fas_float(_target, _value) ({ \
		float nv = _value; \
		uint32_t ov = as_fas_uint32((uint32_t*)_target, *(uint32_t*)&nv); \
		*(float*)&ov; \
	})

/******************************************************************************
 * COMPARE AND SWAP
 *****************************************************************************/

// Note - this assumes pointers are 8 bytes.
// bool as_cas_ptr(void** target, void* old_value, void* new_value)
#define as_cas_ptr(_target, _old_value, _new_value) ({ \
		as_cas_uint64((uint64_t*)_target, (uint64_t)_old_value, \
				(uint64_t)_new_value); \
	})

// bool as_cas_double(double* target, double old_value, double new_value)
#define as_cas_double(_target, _old_value, _new_value) ({ \
		double ov = _old_value; \
		double nv = _new_value; \
		as_cas_uint64((uint64_t*)_target, *(uint64_t*)&ov, *(uint64_t*)&nv); \
	})

// bool as_cas_float(float* target, float old_value, float new_value)
#define as_cas_float(_target, _old_value, _new_value) ({ \
		float ov = _old_value; \
		float nv = _new_value; \
		as_cas_uint32((uint32_t*)_target, *(uint32_t*)&ov, *(uint32_t*)&nv); \
	})
