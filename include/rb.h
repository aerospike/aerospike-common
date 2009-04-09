/*
 *  Citrusleaf Foundation
 *  include/rb.h - red-black tree framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include "digest.h"
#include "pthread.h"


/* SYNOPSIS
 * Normal old red-black trees, guarded by a single mutex
 * */



/* cf_rb_value_destructor()
 * A destructor function prototype for a value */
typedef uint64_t (*cf_rb_value_destructor) (void *v);


/* cf_rb_node
 * A red-black tree node */
struct cf_rb_node_t
{
	enum { CF_RB_BLACK, CF_RB_RED } color;
	cf_digest key;
    pthread_mutex_t VALUE_LOCK;
	void *value;

	struct cf_rb_node_t *left, *right, *parent;
};
typedef struct cf_rb_node_t cf_rb_node;


/* cf_rb_tree
 * A red-black tree */
struct cf_rb_tree_t
{
	pthread_mutex_t lock;

	/* Note: in this implementation of a red-black tree, the "root" node
	 * is not actually the root of the tree: it is merely a placeholder.
	 * The actual root of the tree is this placeholder's left child.  This
	 * is done to eliminate quite a bit of special-case checking for the
	 * root node */
	cf_rb_node *root;
	cf_rb_node *sentinel;
    cf_rb_value_destructor destructor;
	uint32      elements; // no bother in making this atomic, it's not very exact
};
typedef struct cf_rb_tree_t cf_rb_tree;


/* External functions */
extern cf_rb_node *cf_rb_get_insert_vlock(cf_rb_tree *tree, cf_digest key, void *value, pthread_mutex_t **vlock);
extern cf_rb_node *cf_rb_insert(cf_rb_tree *tree, cf_digest key, void *value);
extern cf_rb_node *cf_rb_insert_vlock(cf_rb_tree *tree, cf_digest key, void *value, pthread_mutex_t **vlock);
extern cf_rb_node *cf_rb_search(cf_rb_tree *tree, cf_digest key);
extern cf_rb_node *cf_rb_search_vlock(cf_rb_tree *tree, cf_digest key, pthread_mutex_t **vlock);
extern uint64_t cf_rb_delete(cf_rb_tree *tree, cf_digest key);
extern cf_rb_tree *cf_rb_create(cf_rb_value_destructor destructor);
extern void cf_rb_purge(cf_rb_tree *tree, cf_rb_node *r);
#define cf_rb_reserve(_t) cf_rc_reserve((_t))
extern int cf_rb_release(cf_rb_tree *tree);

extern uint32_t cf_rb_size(cf_rb_tree *tree);
typedef void (*cf_rb_reduce_fn) (cf_digest digest, void *value, void *udata);
extern void cf_rb_reduce(cf_rb_tree *tree, cf_rb_reduce_fn cb, void *udata);
