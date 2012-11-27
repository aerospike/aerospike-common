/*
 *  Citrusleaf Foundation
 *  src/cf_index.c - index abstraction layer
 *
 *  Copyright 2012 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */

#include "cf.h"

/* SYNOPSIS
 * Abstraction to support transparently multiple (secondary) index representations
 * (lists, trees, Bloom filters, ...), with shared functionality
 * (allocation, locking, etc.) provided by this layer.
 */

/*
 * Notes:
 * ------
 *
 *   1). You must call "cf_index_init(IndexTypeParams)" once before creating an index of any type.
 *
 *   2). There is currently no error checking to enforce (1).
 */

/*
 * Internal types and structures.
 */

/* Array of index type parameters (global to an instantiation of the index abstraction layer.) */
/* NB: The order must match that of the "cf_index_type" enum! */
static cf_index_type_parameters *cf_index_type_params = NULL;

/* Method types supported by each index type. */

typedef int (*cf_index_create_fun)(void *v_this, void *v_tdata);
typedef void (*cf_index_destroy_fun)(void *v_this);

typedef int (*cf_index_put_fun)(void *v_this, void *key, void *value, void *v_tdata);
typedef int (*cf_index_put_unique_fun)(void *v_this, void *key, void *value, void *v_tdata);

typedef int (*cf_index_get_fun)(void *v_this, void *key, void *value, void *v_tdata);

typedef int (*cf_index_delete_fun)(void *v_this, void *key, void *v_tdata);
typedef int (*cf_index_get_and_delete_fun)(void *v_this, void *key, void *value, void *v_tdata);

typedef int (*cf_index_reduce_fun)(void *v_this, cf_index_reduce_fn reduce_fn, void *udata, void *v_tdata);
typedef int (*cf_index_reduce_delete_fun)(void *v_this, cf_index_reduce_fn reduce_fn, void *udata, void *v_tdata);

typedef uint32_t (*cf_index_get_size_fun)(void *v_this);

/* Method dispatch table for an index type. */
typedef struct cf_index_type_functions_s {
	cf_index_create_fun create;
	cf_index_destroy_fun destroy;
	cf_index_put_fun put;
	cf_index_put_unique_fun put_unique;
	cf_index_get_fun get;
	cf_index_delete_fun delete;
	cf_index_get_and_delete_fun get_and_delete;
	cf_index_reduce_fun reduce;
	cf_index_reduce_delete_fun reduce_delete;
	cf_index_get_size_fun get_size;
} cf_index_type_functions;

/* Array of method dispatch tables for each index type (representation.) */
/* NB: The order must match that of the "cf_index_type" enum! */
static cf_index_type_functions cf_index_type_funs[CF_INDEX_NUM_TYPES] = {
		/* Type: slist - Simple, Sorted List. */
		{slist_create,
		 slist_destroy,
		 slist_put,
		 slist_put_unique,
		 slist_get,
		 slist_delete,
		 slist_get_and_delete,
		 slist_reduce,
		 slist_reduce_delete,
		 slist_get_size},

		 /* Type: ttree - T-tree. */
		{ttree_create,
		 ttree_destroy,
		 ttree_put,
		 ttree_put_unique,
		 ttree_get,
		 ttree_delete,
		 ttree_get_and_delete,
		 ttree_reduce,
		 ttree_reduce_delete,
		 ttree_get_size}
};


/* Locking functions.*/


static int cf_index_lockpool_create()
{
	// XXX -- TBD
	return CF_INDEX_OK;
}

static void cf_index_lockpool_destroy()
{
	// XXX -- TBD
}


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
int cf_index_init(cf_index_type_parameters *type_params)
{
	if (0 != cf_index_lockpool_create()) {
		return CF_INDEX_ERR;
	}

	cf_index_type_params = type_params;

	return CF_INDEX_OK;
}

/*
 * Terminate an instantiation of the index abstraction layer.
 *
 * Do not use any "cf_index" functions after calling this function, so free your indexes beforehand.
 */
void cf_index_shutdown()
{
	cf_index_type_params = NULL;
	cf_index_lockpool_destroy();
}


/* Index object functions. */


int cf_index_create(cf_index *this, cf_index_type type)
{
	if (0 == (this = cf_malloc(sizeof(cf_index))))
		return CF_INDEX_ERR;

	this->type = type;

	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].create(v_this, v_tdata);
}

void cf_index_destroy(cf_index *this)
{
	void *v_this = (void *) &(this->object);
	cf_index_type_funs[this->type].destroy(v_this);
	cf_free(this);
}

int cf_index_put(cf_index *this, void *key, void *value)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].put(v_this, key, value, v_tdata);
}

int cf_index_put_unique(cf_index *this, void *key, void *value)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].put_unique(v_this, key, value, v_tdata);
}

int cf_index_get(cf_index *this, void *key, void *value)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].get(v_this, key, value, v_tdata);
}

int cf_index_delete(cf_index *this, void *key)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].delete(v_this, key, v_tdata);
}

int cf_index_get_and_delete(cf_index *this, void *key, void *value)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].get_and_delete(v_this, key, value, v_tdata);
}

int cf_index_reduce(cf_index *this, cf_index_reduce_fn reduce_fn, void *udata)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].reduce(v_this, reduce_fn, udata, v_tdata);
}

int cf_index_reduce_delete(cf_index *this, cf_index_reduce_fn reduce_fn, void *udata)
{
	void *v_this = (void *) &(this->object);
	void *v_tdata = cf_index_type_params[this->type];
	return cf_index_type_funs[this->type].reduce_delete(v_this, reduce_fn, udata, v_tdata);
}

uint32_t cf_index_get_size(cf_index *this)
{
	void *v_this = (void *) &(this->object);
	return cf_index_type_funs[this->type].get_size(v_this);
}
