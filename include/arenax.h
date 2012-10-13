/*
 * cf/include/arenax.h
 *
 * An arena that uses persistent memory.
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
#include <sys/types.h>


//==========================================================
// Typedefs & Constants
//

typedef struct cf_arenax_s cf_arenax;

typedef uint32_t cf_arenax_handle;

typedef enum {
	CF_ARENAX_OK = 0,
	CF_ARENAX_ERR_BAD_PARAM,
	CF_ARENAX_ERR_STAGE_CREATE,
	CF_ARENAX_ERR_STAGE_ATTACH,
	CF_ARENAX_ERR_STAGE_DETACH,
	CF_ARENAX_ERR_UNKNOWN
} cf_arenax_err;

#define CF_ARENAX_BIGLOCK	(1 << 0)
#define CF_ARENAX_CALLOC	(1 << 1)

// Stage is indexed by 8 bits.
#define CF_ARENAX_MAX_STAGES (1 << 8) // 256


//==========================================================
// Public API
//

//------------------------------------------------
// Persisted Size (excluding stages)
//
size_t cf_arenax_sizeof();

//------------------------------------------------
// Constructors
//
cf_arenax_err cf_arenax_create(cf_arenax* this, key_t key_base,
		uint32_t element_size, uint32_t stage_capacity, uint32_t max_stages,
		uint32_t flags);
cf_arenax_err cf_arenax_resume(cf_arenax* this, key_t key_base,
		uint32_t element_size, uint32_t stage_capacity, uint32_t max_stages,
		uint32_t flags);

//------------------------------------------------
// Destructor
//
cf_arenax_err cf_arenax_detach(cf_arenax* this);

//------------------------------------------------
// Allocate/Free an Element
//
cf_arenax_handle cf_arenax_alloc(cf_arenax* this);
void cf_arenax_free(cf_arenax* this, cf_arenax_handle h);

//------------------------------------------------
// Convert Handle to Pointer
//
void* cf_arenax_resolve(cf_arenax* this, cf_arenax_handle h);
