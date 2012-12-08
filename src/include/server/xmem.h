/*
 * cf/include/xmem.h
 *
 * Persistent memory API - wraps Linux System V IPC shared memory.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */

#pragma once


//==========================================================
// Includes
//

#include <stddef.h>
#include <sys/types.h>


//==========================================================
// Typedefs & Constants
//

// Must be in-sync with internal array XMEM_ERR_STRINGS[]:
typedef enum {
	CF_XMEM_OK = 0,
	CF_XMEM_ERR_PERMISSION,
	CF_XMEM_ERR_BLOCK_DOES_NOT_EXIST,
	CF_XMEM_ERR_BLOCK_EXISTS,
	CF_XMEM_ERR_BLOCK_ATTACHED,
	CF_XMEM_ERR_BLOCK_SIZE,
	CF_XMEM_ERR_NO_MEMORY,
	CF_XMEM_ERR_SYSTEM_LIMIT,
	CF_XMEM_ERR_UNKNOWN
} cf_xmem_err;


//==========================================================
// Public API
//

//------------------------------------------------
// Get Error Description
//
const char* cf_xmem_errstr(cf_xmem_err err);

//------------------------------------------------
// Create/Destroy
//
cf_xmem_err cf_xmem_create_block(key_t key, size_t size, void** pp_block);
cf_xmem_err cf_xmem_destroy_block(key_t key);

//------------------------------------------------
// Attach/Detach
//
cf_xmem_err cf_xmem_attach_block(key_t key, size_t size, void** pp_block);
cf_xmem_err cf_xmem_detach_block(void* p_block);
