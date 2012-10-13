/*
 * cf/src/arenax.c
 *
 * An arena that uses persistent memory.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */


//==========================================================
// Includes
//

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "xmem.h"

#include "arenax.h"


//==========================================================
// Private "Class Members"
//

//------------------------------------------------
// Constants
//

// Element is indexed by 24 bits.
const uint32_t MAX_STAGE_CAPACITY = 1 << 24; // 16 M

// Limit so stage_size fits in 32 bits.
// (Probably unnecessary - size_t is 64 bits on our systems.)
const uint64_t MAX_STAGE_SIZE = 0xFFFFffff;

//------------------------------------------------
// Typedefs
//

typedef struct handle_s {
	uint32_t stage_id:8;
 	uint32_t element_id:24;
} __attribute__ ((__packed__)) handle;

// TODO - should we bother with this wrapper struct?
typedef struct free_element_s {
	cf_arenax_handle next_h;
	// Not bothering with a "magic" member for now.
} free_element;

//------------------------------------------------
// Function Declarations
//

static cf_arenax_err add_stage(cf_arenax* this);
static cf_arenax_err attach_existing_stages(cf_arenax* this);

//------------------------------------------------
// Data
//

struct cf_arenax_s {
	// Configuration (passed in constructors)
	key_t				key_base;
	uint32_t			element_size;
	uint32_t			stage_capacity;
	uint32_t			max_stages;
	uint32_t			flags;

	// Configuration (derived)
	size_t				stage_size;

	// Free-element List
	cf_arenax_handle	free_h;

	// Where to End-allocate
	uint32_t			at_stage_id;
	uint32_t			at_element_id;

	// Thread Safety
	pthread_mutex_t		lock;

	// Current Stages
	uint32_t			stage_count;
	uint8_t*			stages[CF_ARENAX_MAX_STAGES];
};


//==========================================================
// Public API
//

//------------------------------------------------
// Return persistent memory size needed. Excludes
// stages, which cf_arenax handles internally.
//
size_t
cf_arenax_sizeof()
{
	return sizeof(cf_arenax);
}

//------------------------------------------------
// Create a cf_arenax object in persistent memory.
// Also create and attach the first arena stage in
// persistent memory.
//
cf_arenax_err
cf_arenax_create(cf_arenax* this, key_t key_base, uint32_t element_size,
		uint32_t stage_capacity, uint32_t max_stages, uint32_t flags)
{
	if (stage_capacity == 0) {
		stage_capacity = MAX_STAGE_CAPACITY;
	}
	else if (stage_capacity > MAX_STAGE_CAPACITY) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	if (max_stages == 0) {
		max_stages = CF_ARENAX_MAX_STAGES;
	}
	else if (max_stages > CF_ARENAX_MAX_STAGES) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	uint64_t stage_size = (uint64_t)stage_capacity * (uint64_t)element_size;

	if (stage_size > MAX_STAGE_SIZE) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	this->key_base = key_base;
	this->element_size = element_size;
	this->stage_capacity = stage_capacity;
	this->max_stages = max_stages;
	this->flags = flags;

	this->stage_size = (size_t)stage_size;

	this->free_h = 0;

	// Skip 0:0 so null handle is never used.
	this->at_stage_id = 0;
	this->at_element_id = 1;

	if ((flags & CF_ARENAX_BIGLOCK) &&
			pthread_mutex_init(&this->lock, 0) != 0) {
		return CF_ARENAX_ERR_UNKNOWN;
	}

	this->stage_count = 0;
	memset(this->stages, 0, sizeof(this->stages));

	// Add first stage.
	cf_arenax_err result = add_stage(this);

	// No need to detach - add_stage() won't fail and leave attached stage.
	if (result != CF_ARENAX_OK && (this->flags & CF_ARENAX_BIGLOCK)) {
		pthread_mutex_destroy(&this->lock);
	}

	return result;
}

//------------------------------------------------
// Resume a cf_arenax object in persistent memory.
// Also find and attach all arena stages that were
// in use by this cf_arenax object.
//
// If call returns CF_ARENAX_ERR_STAGE_DETACH,
// persistent memory blocks may still be attached.
// Deleting them has no immediate effect - they
// linger until the server stops. So proceeding in
// this condition and doing a clean start would
// leak persistent memory.
//
cf_arenax_err
cf_arenax_resume(cf_arenax* this, key_t key_base, uint32_t element_size,
		uint32_t stage_capacity, uint32_t max_stages, uint32_t flags)
{
	if (stage_capacity == 0) {
		stage_capacity = MAX_STAGE_CAPACITY;
	}
	else if (stage_capacity > MAX_STAGE_CAPACITY) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	if (max_stages == 0) {
		max_stages = CF_ARENAX_MAX_STAGES;
	}
	else if (max_stages > CF_ARENAX_MAX_STAGES) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	if (this->key_base != key_base ||
		this->element_size != element_size ||
		this->stage_capacity != stage_capacity) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	if (this->stage_count > max_stages) {
		return CF_ARENAX_ERR_BAD_PARAM;
	}

	this->max_stages = max_stages;
	this->flags = flags;

	if ((flags & CF_ARENAX_BIGLOCK) &&
			pthread_mutex_init(&this->lock, 0) != 0) {
		return CF_ARENAX_ERR_UNKNOWN;
	}

	memset(this->stages, 0, sizeof(this->stages));

	// Find all existing stages and rebuild stage pointer table.
	cf_arenax_err result = attach_existing_stages(this);

	if (result != CF_ARENAX_OK) {
		cf_arenax_err detach_result = cf_arenax_detach(this);

		// If detach fails, it's more serious - override result.
		if (detach_result != CF_ARENAX_OK) {
			result = detach_result;
		}
	}

	return result;
}

//------------------------------------------------
// Free internal resources and detach any attached
// stages. Don't call (as public API) after failed
// cf_arenax_create() or cf_arenax_resume() call -
// those functions clean up on failure.
//
cf_arenax_err
cf_arenax_detach(cf_arenax* this)
{
	if (this->flags & CF_ARENAX_BIGLOCK) {
		pthread_mutex_destroy(&this->lock);
	}

	cf_arenax_err result = CF_ARENAX_OK;

	for (uint32_t i = 0; i < this->stage_count; i++) {
		uint8_t* p_stage = this->stages[i];

		if (! p_stage) {
			// Happens if we got part way through attaching stages then failed.
			break;
		}

		if (cf_xmem_detach_block((void*)p_stage) != CF_XMEM_OK) {
			result = CF_ARENAX_ERR_STAGE_DETACH;
			// Something really out-of-whack, but keep going...
		}
	}

	return result;
}

//------------------------------------------------
// Allocate an element within the arena.
//
cf_arenax_handle
cf_arenax_alloc(cf_arenax* this)
{
	if ((this->flags & CF_ARENAX_BIGLOCK) &&
			pthread_mutex_lock(&this->lock) != 0) {
		return 0;
	}

	cf_arenax_handle h;

	// Check free list first.
	if (this->free_h != 0) {
		h = this->free_h;

		free_element* p_free_element = cf_arenax_resolve(this, h);

		this->free_h = p_free_element->next_h;
	}
	// Otherwise keep end-allocating.
	else {
		if (this->at_element_id >= this->stage_capacity) {
			if (add_stage(this) != CF_ARENAX_OK) {
				if (this->flags & CF_ARENAX_BIGLOCK) {
					pthread_mutex_unlock(&this->lock);
				}

				return 0;
			}

			this->at_stage_id++;
			this->at_element_id = 0;
		}

		((handle*)&h)->stage_id = this->at_stage_id;
		((handle*)&h)->element_id = this->at_element_id;

		this->at_element_id++;
	}

	if (this->flags & CF_ARENAX_BIGLOCK) {
		pthread_mutex_unlock(&this->lock);
	}

	if (this->flags & CF_ARENAX_CALLOC) {
		memset(cf_arenax_resolve(this, h), 0, this->element_size);
	}

	return h;
}

//------------------------------------------------
// Free an element.
//
void
cf_arenax_free(cf_arenax* this, cf_arenax_handle h)
{
	free_element* p_free_element = cf_arenax_resolve(this, h);

	if ((this->flags & CF_ARENAX_BIGLOCK) &&
			pthread_mutex_lock(&this->lock) != 0) {
		// TODO - function doesn't return failure - just press on?
		return;
	}

	p_free_element->next_h = this->free_h;
	this->free_h = h;

	if (this->flags & CF_ARENAX_BIGLOCK) {
		pthread_mutex_unlock(&this->lock);
	}
}

//------------------------------------------------
// Convert cf_arenax_handle to memory address.
//
void*
cf_arenax_resolve(cf_arenax* this, cf_arenax_handle h)
{
	return this->stages[((handle*)&h)->stage_id] +
			(((handle*)&h)->element_id * this->element_size);
}


//==========================================================
// Private Functions
//

//------------------------------------------------
// Create and attach a persistent memory block,
// and store its pointer in the stages array.
//
static cf_arenax_err
add_stage(cf_arenax* this)
{
	if (this->stage_count >= this->max_stages) {
		return CF_ARENAX_ERR_STAGE_CREATE;
	}

	cf_xmem_err result = cf_xmem_create_block(
			this->key_base + this->stage_count, this->stage_size,
			(void**)&this->stages[this->stage_count]);

	if (result != CF_XMEM_OK) {
		// TODO - log cf_xmem_errstr(result)?
		return CF_ARENAX_ERR_STAGE_CREATE;
	}

	this->stage_count++;

	return CF_ARENAX_OK;
}

//------------------------------------------------
// Find and attach all persistent memory blocks
// that were in use.
//
static cf_arenax_err
attach_existing_stages(cf_arenax* this)
{
	for (uint32_t i = 0; i < this->stage_count; i++) {
		cf_xmem_err result = cf_xmem_attach_block(this->key_base + i,
				this->stage_size, (void**)&this->stages[i]);

		if (result != CF_XMEM_OK) {
			// TODO - log cf_xmem_errstr(result)?
			return CF_ARENAX_ERR_STAGE_ATTACH;
		}
	}

	return CF_ARENAX_OK;
}
