/*
 * cf/include/vmap.h
 *
 * A vector of fixed-size objects, also accessible by name.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */

#pragma once


//==========================================================
// Includes
//

#include <stdint.h>


//==========================================================
// Typedefs
//

typedef struct cf_vmap_s cf_vmap;

typedef enum {
	CF_VMAP_OK = 0,
	CF_VMAP_ERR_BAD_PARAM,
	CF_VMAP_ERR_FULL,
	CF_VMAP_ERR_NAME_EXISTS,
	CF_VMAP_ERR_NAME_NOT_FOUND,
	CF_VMAP_ERR_OUT_OF_MEMORY,
	CF_VMAP_ERR_UNKNOWN
} cf_vmap_err;


//==========================================================
// Public API
//

cf_vmap_err cf_vmap_create(uint32_t value_size, uint32_t max_count,
		uint32_t hash_size, uint32_t max_name_size, cf_vmap** pp_vmap);
void cf_vmap_destroy(cf_vmap* this);

uint32_t cf_vmap_count(cf_vmap* this);
cf_vmap_err cf_vmap_get_by_index(cf_vmap* this, uint32_t index,
		void** pp_value);
cf_vmap_err cf_vmap_get_by_name(cf_vmap* this, const char* name,
		void** pp_value);
cf_vmap_err cf_vmap_get_index(cf_vmap* this, const char* name,
		uint32_t* p_index);
cf_vmap_err cf_vmap_put_unique(cf_vmap* this, const char* name,
		const void* p_value, uint32_t* p_index);
