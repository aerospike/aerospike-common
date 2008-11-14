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


/* SYNOPSIS
 * Normal old red-black trees, guarded by a single mutex
 * */


                                                                          
/* cf_rb_node
 * A red-black tree node */
struct cf_rb_node_t
{
	enum { CF_RB_BLACK, CF_RB_RED } color;
	cf_digest key;
	void *value;

	struct cf_rb_node_t *left, *right, *parent;
};
typedef struct cf_rb_node_t cf_rb_node;


/* cf_rb_tree
 * A red-black tree */
struct cf_rb_tree_t
{
	pthread_mutex_t TREE_LOCK;

	/* Note: in this implementation of a red-black tree, the "root" node
	 * is not actually the root of the tree: it is merely a placeholder.
	 * The actual root of the tree is this placeholder's left child.  This
	 * is done to eliminate quite a bit of special-case checking for the
	 * root node */
	cf_rb_node *root;
	cf_rb_node *sentinel;
	uint32      elements; // no bother in making this atomic, it's not very exact
};
typedef struct cf_rb_tree_t cf_rb_tree;


/* External functions */
extern cf_rb_node *cf_rb_insert(cf_rb_tree *tree, cf_digest key, void *value);
extern cf_rb_node *cf_rb_search(cf_rb_tree *tree, cf_digest key);
extern void cf_rb_delete(cf_rb_tree *tree, cf_digest key);
extern cf_rb_tree *cf_rb_create();
extern void cf_rb_purge(cf_rb_tree *tree, cf_rb_node *r);
extern void cf_rb_destroy(cf_rb_tree *tree);

typedef void (*cf_rb_reduce_fn) (cf_digest digest, void *value, void *udata);
extern void cf_rb_reduce(cf_rb_tree *tree, cf_rb_reduce_fn cb, void *udata);
