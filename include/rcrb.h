/*
 *  Citrusleaf Foundation
 *  include/rb.h - red-black tree framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once

#include <pthread.h>
#include "digest.h"
#include "olock.h"


/* SYNOPSIS
 * Normal old red-black trees, guarded by a single mutex
 * */


/* cf_rcrb_value_destructor()
 * A destructor function prototype for a value */
typedef void (*cf_rcrb_value_destructor) (void *v, void *udata);


/* cf_rcrb_node
 * A red-black tree node */
struct cf_rcrb_node_t
{
	// these fields get hit constantly during the iteration
	enum { CF_RCRB_BLACK, CF_RCRB_RED } color;
	
	cf_digest key;
	
	struct cf_rcrb_node_t *left, *right, *parent;
	
	void *value;
};
typedef struct cf_rcrb_node_t cf_rcrb_node;

                                                                     
/* cf_rcrb_tree
 * A red-black tree */
struct cf_rcrb_tree_t
{
	pthread_mutex_t lock;

	/* Note: in this implementation of a red-black tree, the "root" node
	 * is not actually the root of the tree: it is merely a placeholder.
	 * The actual root of the tree is this placeholder's left child.  This
	 * is done to eliminate quite a bit of special-case checking for the
	 * root node */
	cf_rcrb_node *root;
	cf_rcrb_node *sentinel;
	
    cf_rcrb_value_destructor destructor;
    void 					*destructor_udata;
    
	uint32_t      elements; // no bother in making this atomic, it's not very exact
};
typedef struct cf_rcrb_tree_t cf_rcrb_tree;

// Note on interfaces:
// when inserting, the reference count of the value is *consumed* by the insert
// if it succeeds
//
// Except for get-insert, which is a little weird
// if a value is returned (thus a different value is gotten), its refcount is increased
// If the value is inserted (and not returend), the refcount is consumed
//
// What's cool about the insert and get_insert functions is you can avoid a double-search
// in the cases where you want to 


/* External functions */

extern cf_rcrb_node * cf_rcrb_get_insert_vlock(cf_rcrb_tree *tree, cf_digest *key, pthread_mutex_t **vlock);
extern cf_rcrb_node * cf_rcrb_insert_vlock(cf_rcrb_tree *tree, cf_digest *key, pthread_mutex_t **vlock);

extern void *cf_rcrb_search(cf_rcrb_tree *tree, cf_digest *key);
// delete- 0 is ok, -1 is fail, -2 is key not found
extern int cf_rcrb_delete(cf_rcrb_tree *tree, cf_digest *key);

extern cf_rcrb_tree *cf_rcrb_create(cf_rcrb_value_destructor destructor, void *destructor_udata);

extern void cf_rcrb_purge(cf_rcrb_tree *tree, cf_rcrb_node *r);
#define cf_rcrb_reserve(_t) cf_rc_reserve((_t))
extern int cf_rcrb_release(cf_rcrb_tree *tree, void *destructor_udata);

extern uint32_t cf_rcrb_size(cf_rcrb_tree *tree); // number of elements

typedef void (*cf_rcrb_reduce_fn) (cf_digest *key, void *value, void *udata);

// reduce gives a reference count of the value: you must release it
// and it contains, internally, code to not block the tree lock - so you can spend as much time
// in the reduce function as you want
extern void cf_rcrb_reduce(cf_rcrb_tree *tree, cf_rcrb_reduce_fn cb, void *udata);
// reduce_sync doesn't take a reference, and holds the tree lock
extern void cf_rcrb_reduce_sync(cf_rcrb_tree *tree, cf_rcrb_reduce_fn cb, void *udata);
