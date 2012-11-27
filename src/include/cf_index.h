/*
 *  Citrusleaf Foundation
 *  include/cf_index.h - index abstraction layer
 *
 *  Copyright 2012 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */

#pragma once

/* SYNOPSIS
 * Abstraction to support transparently multiple (secondary) index representations
 * (lists, trees, Bloom filters, ...), with shared functionality
 * (allocation, locking, etc.) provided by this layer.
 */


/* Index abstraction types. */


/* Supported index types. */
typedef enum __attribute__((__packed__)) cf_index_type_e {
	CF_INDEX_TYPE_SLIST = 0,
	CF_INDEX_TYPE_TTREE,
	CF_INDEX_NUM_TYPES          /* Total number of implemented index types. */
	// Future index types:
	// CF_INDEX_TYPE_ELIST,        /* (Not yet implemented.) */
	// CF_INDEX_TYPE_BLOOM_FILTER  /* (Not yet implemented.) */
} cf_index_type;

/* Array of index type parameters (global to an instantiation of the index abstraction layer.) */
/* NB: The order must match that of the "cf_index_type" enum! */
typedef void *cf_index_type_parameters[CF_INDEX_NUM_TYPES];

/* Index abstraction. */
/*
 * NB: This minimal structure is intentionally sized to 12 bytes for packing efficiency.
 * Each index type implementation is free to use use whatever portionof the 11-byte
 * per-instance object data as needed for its internal representation.
 */
typedef struct cf_index_s {
	cf_index_type type;        /* Type of index, e.g., list, tree, etc. */
	uint8_t object[11];        /* Type-specific private data used by the representation. */
} __attribute__((__packed__)) cf_index;

/* Return status codes for index object functions. */
typedef enum cf_index_status_e {
	CF_INDEX_ERR_FOUND = -3,
	CF_INDEX_ERR_NOTFOUND = -2,
	CF_INDEX_ERR = -1,
	CF_INDEX_OK = 0
} cf_index_status;

/* Type for a reduce function to be mapped over all elements in an index. */
typedef int (*cf_index_reduce_fn)(void *key, void *value, void *udata);


/* Index abstraction layer functions. */


/*
 * Initialize an instantiation of the index abstraction layer
 * using the array of index type-specific parameters passed in.
 *
 * All indexes created during this instantiation will use these type-specific
 * parameters (e.g., maximum data structure sizes, allocation policies, and any
 * other tuning parameters.)
 *
 * Call once before creating any type of index object.
 */
extern int cf_index_init(cf_index_type_parameters *type_params);

/*
 * Terminate an instantiation of the index abstraction layer.
 *
 * Do not use any "cf_index" functions after calling this function, so free your indexes beforehand.
 */
extern void cf_index_shutdown();


/* Index object functions. */


extern int cf_index_create(cf_index *this, cf_index_type type);
extern void cf_index_destroy(cf_index *this);

extern int cf_index_put(cf_index *this, void *key, void *value);
extern int cf_index_put_unique(cf_index *this, void *key, void *value);

extern int cf_index_get(cf_index *this, void *key, void *value);

extern int cf_index_delete(cf_index *this, void *key);
extern int cf_index_get_and_delete(cf_index *this, void *key, void *value);

extern int cf_index_reduce(cf_index *this, cf_index_reduce_fn reduce_fn, void *udata);
extern int cf_index_reduce_delete(cf_index *this, cf_index_reduce_fn reduce_fn, void *udata);

extern uint32_t cf_index_get_size(cf_index *this);
