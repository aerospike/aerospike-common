/*
 * cf/src/xmem.c
 *
 * Persistent memory API - wraps Linux System V IPC shared memory.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */


//==========================================================
// Includes
//
#include "server/xmem.h"

#include <errno.h>
#include <stddef.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>



//==========================================================
// Constants
//

const int SHMGET_FLAGS_CREATE_ONLY = IPC_CREAT | IPC_EXCL | 0666;
const int SHMGET_FLAGS_EXISTING = 0666;

// Must be in-sync with cf_xmem_err:
const char* XMEM_ERR_STRINGS[] = {
	"ok",
	"permissions error",
	"block does not exist",
	"block exists",
	"block is attached",
	"block size error",
	"no memory",
	"reached shared memory system limit",
	"unknown error"
};


//==========================================================
// Forward Declarations
//

static inline cf_xmem_err shmat_errno_to_xmem_err();
static inline cf_xmem_err shmctl_rmid_errno_to_xmem_err();
static inline cf_xmem_err shmctl_stat_errno_to_xmem_err();
static inline cf_xmem_err shmget_errno_to_xmem_err();


//==========================================================
// Public API
//

//------------------------------------------------
// Convert cf_xmem_err value to meaningful string.
//
const char*
cf_xmem_errstr(cf_xmem_err err)
{
	if (err < 0 || err > CF_XMEM_ERR_UNKNOWN) {
		err = CF_XMEM_ERR_UNKNOWN;
	}

	return XMEM_ERR_STRINGS[err];
}

//------------------------------------------------
// Create a new persistent memory block and attach
// to it. Fail if a block already exists for
// specified key.
//
cf_xmem_err
cf_xmem_create_block(key_t key, size_t size, void** pp_block)
{
	// Create the block if it doesn't exist, fail if it does.
	int shmid = shmget(key, size, SHMGET_FLAGS_CREATE_ONLY);

	if (shmid < 0) {
		return shmget_errno_to_xmem_err();
	}

	void* p_block = shmat(shmid, NULL, 0);

	if (p_block == (void*)-1) {
		return shmat_errno_to_xmem_err();
	}

	*pp_block = p_block;

	return CF_XMEM_OK;
}

//------------------------------------------------
// Destroy a persistent memory block. Fail if
// block is currently attached. Returns
// CF_XMEM_ERR_BLOCK_DOES_NOT_EXIST if no block
// exists for specified key - caller interprets
// whether or not this is an error.
//
cf_xmem_err
cf_xmem_destroy_block(key_t key)
{
	// Attempt to get shmid for block - don't create if it doesn't exist. Pass
	// __size of 1 to be sure we get existing block whatever size it is.
	int shmid = shmget(key, 1, SHMGET_FLAGS_EXISTING);

	if (shmid < 0) {
		// Includes case where block doesn't exist.
		return shmget_errno_to_xmem_err();
	}

	// Block exists - remove it unless it's attached.

	// Check if block is attached.
	struct shmid_ds ds;
	int result = shmctl(shmid, IPC_STAT, &ds);

	if (result < 0) {
		return shmctl_stat_errno_to_xmem_err();
	}

	// Fail if block is attached.
	if (ds.shm_nattch > 0) {
		return CF_XMEM_ERR_BLOCK_ATTACHED;
	}

	// Remove the block.
	result = shmctl(shmid, IPC_RMID, NULL);

	if (result != 0) {
		return shmctl_rmid_errno_to_xmem_err();
	}

	return CF_XMEM_OK;
}

//------------------------------------------------
// Attach to existing persistent memory block.
// Fail if no block exists for specified key.
//
cf_xmem_err
cf_xmem_attach_block(key_t key, size_t size, void** pp_block)
{
	// Attempt to get shmid for block - don't create if it doesn't exist.
	int shmid = shmget(key, size, SHMGET_FLAGS_EXISTING);

	if (shmid < 0) {
		return shmget_errno_to_xmem_err();
	}

	void* p_block = shmat(shmid, NULL, 0);

	if (p_block == (void*)-1) {
		return shmat_errno_to_xmem_err();
	}

	*pp_block = p_block;

	return CF_XMEM_OK;
}

//------------------------------------------------
// Detach from existing persistent memory block.
//
cf_xmem_err
cf_xmem_detach_block(void* p_block)
{
	int result = shmdt(p_block);

	// Only fails if p_block doesn't correspond to attached block.
	if (result < 0) {
		return CF_XMEM_ERR_UNKNOWN;
	}

	return CF_XMEM_OK;
}


//==========================================================
// Helpers
//

//------------------------------------------------
// Convert shmctl(IPC_RMID) errno to cf_xmem_err.
//
static inline cf_xmem_err
shmctl_rmid_errno_to_xmem_err()
{
	switch (errno) {
	case EPERM:
		return CF_XMEM_ERR_PERMISSION;
	case EIDRM:
	case EINVAL:
		// These should never happen given how we call shmctl(IPC_RMID).
	default:
		return CF_XMEM_ERR_UNKNOWN;
	}
}

//------------------------------------------------
// Convert shmctl(IPC_STAT) errno to cf_xmem_err.
//
static inline cf_xmem_err
shmctl_stat_errno_to_xmem_err()
{
	switch (errno) {
	case EACCES:
		return CF_XMEM_ERR_PERMISSION;
	case EFAULT:
	case EIDRM:
	case EINVAL:
	case EOVERFLOW:
		// These should never happen given how we call shmctl(IPC_STAT).
	default:
		return CF_XMEM_ERR_UNKNOWN;
	}
}

//------------------------------------------------
// Convert shmat() errno to cf_xmem_err.
//
static inline cf_xmem_err
shmat_errno_to_xmem_err()
{
	switch (errno) {
	case EACCES:
		return CF_XMEM_ERR_PERMISSION;
	case ENOMEM:
		return CF_XMEM_ERR_NO_MEMORY;
	case EINVAL:
		// This should never happen given how we use shmat().
	default:
		return CF_XMEM_ERR_UNKNOWN;
	}
}

//------------------------------------------------
// Convert shmget() errno to cf_xmem_err.
//
static inline cf_xmem_err
shmget_errno_to_xmem_err()
{
	switch (errno) {
	case EACCES:
	case EPERM: // TODO - investigate SHM_HUGETLB
		return CF_XMEM_ERR_PERMISSION;
	case ENOENT:
		return CF_XMEM_ERR_BLOCK_DOES_NOT_EXIST;
	case EEXIST:
		return CF_XMEM_ERR_BLOCK_EXISTS;
	case EINVAL:
		return CF_XMEM_ERR_BLOCK_SIZE;
	case ENOMEM:
		return CF_XMEM_ERR_NO_MEMORY;
	case ENFILE:
	case ENOSPC:
		return CF_XMEM_ERR_SYSTEM_LIMIT;
	default:
		return CF_XMEM_ERR_UNKNOWN;
	}
}
