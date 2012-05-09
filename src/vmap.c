/*
 * cf/src/vmap.c
 *
 * A vector of fixed-size values, also accessible by name.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */


//==========================================================
// Includes
//

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "alloc.h"
#include "atomic.h"
#include "cf.h" // for cf_hash_fnv - yuck
#include "shash.h"

#include "vmap.h"


//==========================================================
// Private "Class Members"
//

//------------------------------------------------
// Function Declarations
//

static bool get_index(cf_vmap* this, const char* name, uint32_t* p_index);
static inline uint32_t hash_fn(void* p_key);
static bool increase_capacity(cf_vmap* this);
static inline void* value_ptr(cf_vmap* this, uint32_t index);


//------------------------------------------------
// Data
//

struct cf_vmap_s {
	// Vector-related
	uint32_t		value_size;
	uint32_t		max_count;
	uint32_t		capacity;
	cf_atomic32		count;
	void*			values;

	// Hash-related
	shash*			p_hash;
	uint32_t		key_size;

	// Generic
	pthread_mutex_t	write_lock;
};

//------------------------------------------------
// Constants
//

const uint32_t INIT_CAPACITY = 64;


//==========================================================
// Public API
//

//------------------------------------------------
// Create a cf_vmap object.
//
cf_vmap_err
cf_vmap_create(uint32_t value_size, uint32_t max_count, uint32_t hash_size,
		uint32_t max_name_size, cf_vmap** pp_vmap)
{
	// Value-size needs to be a multiple of 4 bytes for thread safety.
	if ((value_size & 3) || ! max_count || ! hash_size || ! max_name_size ||
			! pp_vmap) {
		return CF_VMAP_ERR_BAD_PARAM;
	}

	cf_vmap* this = (cf_vmap*)cf_malloc(sizeof(cf_vmap));

	if (! this) {
		return CF_VMAP_ERR_OUT_OF_MEMORY;
	}

	memset(this, 0, sizeof(cf_vmap));

	pthread_mutex_init(&this->write_lock, 0);

	this->value_size = value_size;
	this->max_count = max_count;
	this->capacity = max_count > INIT_CAPACITY ? INIT_CAPACITY : max_count;

	if ((this->values = cf_malloc(this->value_size * this->capacity)) == NULL) {
		cf_vmap_destroy(this);

		return CF_VMAP_ERR_OUT_OF_MEMORY;
	}

	if (shash_create(&this->p_hash, hash_fn, max_name_size, sizeof(uint32_t),
			hash_size, SHASH_CR_MT_MANYLOCK) != SHASH_OK) {
		cf_vmap_destroy(this);

		return CF_VMAP_ERR_UNKNOWN;
	}

	this->key_size = max_name_size;

	*pp_vmap = this;

	return CF_VMAP_OK;
}

//------------------------------------------------
// Destroy a cf_vmap object.
//
void
cf_vmap_destroy(cf_vmap* this)
{
	if (this->values) {
		cf_free(this->values);
	}

	if (this->p_hash) {
		shash_destroy(this->p_hash);
	}

	pthread_mutex_destroy(&this->write_lock);

	cf_free(this);
}

//------------------------------------------------
// Return count.
//
uint32_t
cf_vmap_count(cf_vmap* this)
{
	return cf_atomic32_get(this->count);
}

//------------------------------------------------
// Get value by index.
//
cf_vmap_err
cf_vmap_get_by_index(cf_vmap* this, uint32_t index, void** pp_value)
{
	if (index >= cf_atomic32_get(this->count)) {
		return CF_VMAP_ERR_BAD_PARAM;
	}

	*pp_value = value_ptr(this, index);

	return CF_VMAP_OK;
}

//------------------------------------------------
// Get value by name.
//
cf_vmap_err
cf_vmap_get_by_name(cf_vmap* this, const char* name, void** pp_value)
{
	uint32_t index;

	if (! get_index(this, name, &index)) {
		return CF_VMAP_ERR_NAME_NOT_FOUND;
	}

	*pp_value = value_ptr(this, index);

	return CF_VMAP_OK;
}

//------------------------------------------------
// Get index by name. May pass null p_index to
// just check existence.
//
cf_vmap_err
cf_vmap_get_index(cf_vmap* this, const char* name, uint32_t* p_index)
{
	return get_index(this, name, p_index) ?
			CF_VMAP_OK : CF_VMAP_ERR_NAME_NOT_FOUND;
}

//------------------------------------------------
// If name is not found, add new value and return
// newly assigned index (with CF_VMAP_OK). If name
// is found, return index for existing name (with
// CF_VMAP_ERR_NAME_EXISTS) but ignore new value.
// May pass null p_index.
//
cf_vmap_err
cf_vmap_put_unique(cf_vmap* this, const char* name, const void* p_value,
		uint32_t* p_index)
{
	// Not using get_index() since we may need key for shash_put() call.
	char key[this->key_size];

	// Pad with nulls to achieve consistent key.
	strncpy(key, name, this->key_size);

	pthread_mutex_lock(&this->write_lock);

	// If name is found, return existing name's index, ignore p_value.
	if (shash_get(this->p_hash, key, p_index) == SHASH_OK) {
		pthread_mutex_unlock(&this->write_lock);

		return CF_VMAP_ERR_NAME_EXISTS;
	}

	uint32_t count = cf_atomic32_get(this->count);

	// Not allowed to add more.
	if (count >= this->max_count) {
		pthread_mutex_unlock(&this->write_lock);

		return CF_VMAP_ERR_FULL;
	}

	// Add to vector.
	if (count >= this->capacity && ! increase_capacity(this)) {
		pthread_mutex_unlock(&this->write_lock);

		return CF_VMAP_ERR_OUT_OF_MEMORY;
	}

	memcpy(value_ptr(this, count), p_value, this->value_size);

	// Increment count here so indexes returned by other public API calls (just
	// after adding to hash below) are guaranteed to be valid.
	cf_atomic32_incr(&this->count);

	// Add to hash.
	if (shash_put(this->p_hash, key, &count) != SHASH_OK) {
		cf_atomic32_decr(&this->count);

		pthread_mutex_unlock(&this->write_lock);

		return CF_VMAP_ERR_UNKNOWN;
	}

	pthread_mutex_unlock(&this->write_lock);

	if (p_index) {
		*p_index = count;
	}

	return CF_VMAP_OK;
}


//==========================================================
// Private Functions
//

//------------------------------------------------
// Get index by trusted name.
//
static bool
get_index(cf_vmap* this, const char* name, uint32_t* p_index)
{
	char key[this->key_size];

	// Pad with nulls to achieve consistent key.
	strncpy(key, name, this->key_size);

	return shash_get(this->p_hash, key, &p_index) == SHASH_OK;
}

//------------------------------------------------
// Hash a name string.
//
static inline uint32_t
hash_fn(void* p_key)
{
	return (uint32_t)cf_hash_fnv(p_key, strlen((const char*)p_key));
}

//------------------------------------------------
// Increase vector allocation - must be called
// within write-lock, with capacity < max_count.
//
static bool
increase_capacity(cf_vmap* this)
{
	uint32_t capacity = this->capacity << 1;

	if (capacity > this->max_count) {
		capacity = this->max_count;
	}

	void* values = cf_realloc(this->values, this->value_size * capacity);

	// It may be overly optimistic to assume a failed realloc leaves the
	// original allocation intact, but let's do so anyway...

	if (values) {
		this->capacity = capacity;
		this->values = values;
	}

	return values != NULL;
}

//------------------------------------------------
// Return value pointer at trusted index.
//
static inline void*
value_ptr(cf_vmap* this, uint32_t index)
{
	return (void*)((uint8_t*)this->values + (this->value_size * index));
}
