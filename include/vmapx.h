/*
 * cf/include/vmapx.h
 *
 * A vector of fixed-size values, also accessible by name, which operates in
 * persistent memory.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */

#pragma once


//==========================================================
// Includes
//

#include <stddef.h>
#include <stdint.h>


//==========================================================
// Typedefs
//

typedef struct cf_vmapx_s cf_vmapx;

typedef enum {
	CF_VMAPX_OK = 0,
	CF_VMAPX_ERR_BAD_PARAM,
	CF_VMAPX_ERR_FULL,
	CF_VMAPX_ERR_NAME_EXISTS,
	CF_VMAPX_ERR_NAME_NOT_FOUND,
	CF_VMAPX_ERR_UNKNOWN
} cf_vmapx_err;


//==========================================================
// Public API
//

//------------------------------------------------
// Persistent Size (cf_vmapx struct + values)
//
size_t cf_vmapx_sizeof(uint32_t value_size, uint32_t max_count);

//------------------------------------------------
// Constructors
//
cf_vmapx_err cf_vmapx_create(cf_vmapx* this, uint32_t value_size,
		uint32_t max_count, uint32_t hash_size, uint32_t max_name_size);
cf_vmapx_err cf_vmapx_resume(cf_vmapx* this, uint32_t value_size,
		uint32_t max_count, uint32_t hash_size, uint32_t max_name_size);

//------------------------------------------------
// Destructor
//
void cf_vmapx_release(cf_vmapx* this);


//------------------------------------------------
// Number of Values
//
uint32_t cf_vmapx_count(cf_vmapx* this);

//------------------------------------------------
// Get a Value
//
cf_vmapx_err cf_vmapx_get_by_index(cf_vmapx* this, uint32_t index,
		void** pp_value);
cf_vmapx_err cf_vmapx_get_by_name(cf_vmapx* this, const char* name,
		void** pp_value);

//------------------------------------------------
// Get Index from Name
//
cf_vmapx_err cf_vmapx_get_index(cf_vmapx* this, const char* name,
		uint32_t* p_index);

//------------------------------------------------
// Add a Value (if name is unique)
//
cf_vmapx_err cf_vmapx_put_unique(cf_vmapx* this, const void* p_value,
		uint32_t* p_index);
