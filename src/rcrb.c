/*
 *  Citrusleaf Foundation
 *  src/rb.c - red-black trees
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>	
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "cf.h"


/* cf_rcrb_rotate_left
 * Rotate a tree left */
void
cf_rcrb_rotate_left(cf_rcrb_tree *tree, cf_rcrb_node *r)
{
    cf_rcrb_node *s = r->right;

    /* Establish r->right */
    r->right = s->left;
    if (s->left != tree->sentinel)
        s->left->parent = r;

    /* Establish the new parent */
    s->parent = r->parent;
    if (r == r->parent->left)
        r->parent->left = s;
    else
        r->parent->right = s;

    /* Tidy up the pointers */
    s->left = r;
    r->parent = s;
}


/* cf_rcrb_rotate_right
 * Rotate a tree right */
void
cf_rcrb_rotate_right(cf_rcrb_tree *tree, cf_rcrb_node *r)
{
    cf_rcrb_node *s = r->left;

    /* Establish r->left */
    r->left = s->right;
    if (s->right != tree->sentinel)
        s->right->parent = r;

    /* Establish the new parent */
    s->parent = r->parent;
    if (r == r->parent->left)
        r->parent->left = s;
    else
        r->parent->right = s;

    /* Tidy up the pointers */
    s->right = r;
    r->parent = s;
}


/* cf_rcrb_insert
 * Insert a node with a given key into a red-black tree */
int
cf_rcrb_insert(cf_rcrb_tree *tree, cf_digest key, void *value)
{
    cf_rcrb_node *n, *s, *t, *u;

    /* Allocate memory for the new node and set the node parameters */
	// this could be done later, but doing the malloc ahead of the tree lock
	// increases parallelism and decreases lock hold times
    if (NULL == (n = (cf_rcrb_node *)malloc(sizeof(cf_rcrb_node))))
        return(-1);
    n->color = CF_RCRB_RED;
	n->key = key;
	n->value = value;
//    n->left = n->right = tree->sentinel;
// left, right, parent are set during insert
	
    u = n;

    /* Lock the tree */
	pthread_mutex_lock(&tree->lock);

    /* find the place to insert, via the typical method of
     * binary tree insertion */
    n->left = n->right = tree->sentinel;
    s = tree->root;
    t = tree->root->left;
    while (t != tree->sentinel) {
        s = t;
		int c = cf_digest_compare(&key, &t->key);
        if (c)
            t = (c > 0) ? t->left : t->right;
        else
            break;
    }
    n->parent = s;

    /* If the node already exists, stop a double-insertion */
    if ((s != tree->root) && (0 == cf_digest_compare(&n->key, &s->key))) {
        free(n);
		pthread_mutex_unlock(&tree->lock);
        return(-2);
    }

    /* Insert the node */
    if ((s == tree->root) || (0 < cf_digest_compare(&n->key, &s->key)))
        s->left = n;
    else
        s->right = n;

    /* Rebalance the tree */
    while (CF_RCRB_RED == n->parent->color) {
        if (n->parent == n->parent->parent->left) {
            s = n->parent->parent->right;
            if (CF_RCRB_RED == s->color) {
                n->parent->color = CF_RCRB_BLACK;
                s->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    cf_rcrb_rotate_left(tree, n);
                }
                n->parent->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                cf_rcrb_rotate_right(tree, n->parent->parent);
            }
        } else {
            s = n->parent->parent->left;
            if (CF_RCRB_RED == s->color) {
                n->parent->color = CF_RCRB_BLACK;
                s->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    cf_rcrb_rotate_right(tree, n);
                }
                n->parent->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                cf_rcrb_rotate_left(tree, n->parent->parent);
            }
        }
    }
    tree->root->left->color = CF_RCRB_BLACK;
	tree->elements++;
	
	cf_rc_reserve(u->value);

	pthread_mutex_unlock(&tree->lock);
	
    return(0);
}




/* cf_rcrb_get_insert
 * Get or insert a node with a given tree into a red-black tree.
 * */
void *
cf_rcrb_get_insert(cf_rcrb_tree *tree, cf_digest key, void *value)
{
    cf_rcrb_node *n, *s, *t, *u;

    /* Lock the tree */
	pthread_mutex_lock(&tree->lock);

    /* Insert the node directly into the tree, via the typical method of
     * binary tree insertion */
    s = tree->root;
    t = tree->root->left;
	cf_debug(CF_RB,"get-insert: key %"PRIx64" sentinal %p",*(uint64_t *)&key, tree->sentinel);

    while (t != tree->sentinel) {
        s = t;
//		cf_debug(CF_RB,"  at %p: key %"PRIx64": right %p left %p",t,*(uint64_t *)&t->key,t->right,t->left);

		int c = cf_digest_compare(&key, &t->key);
        if (c)
            t = (c > 0) ? t->left : t->right;
        else
			break;
    }

    /* If the node already exists, stop a double-insertion */
    if ((s != tree->root) && (0 == cf_digest_compare(&key, &s->key))) {

    	value = s->value;
		cf_rc_reserve(value);
		pthread_mutex_unlock(&tree->lock);
        return(value);
        
    }

	cf_debug(CF_RB,"get-insert: not found");
	
    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rcrb_node *)malloc(sizeof(cf_rcrb_node)))) {
		cf_debug(CF_RB," malloc failed ");
        return(NULL);
	}
	n->key = key;
	n->value = value;
    n->left = n->right = tree->sentinel;
    n->color = CF_RCRB_RED;
    n->parent = s;
    u = n;

    /* Insert the node */
    if ((s == tree->root) || (0 < cf_digest_compare(&n->key, &s->key)))
        s->left = n;
    else
        s->right = n;

    /* Rebalance the tree */
    while (CF_RCRB_RED == n->parent->color) {
        if (n->parent == n->parent->parent->left) {
            s = n->parent->parent->right;
            if (CF_RCRB_RED == s->color) {
                n->parent->color = CF_RCRB_BLACK;
                s->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    cf_rcrb_rotate_left(tree, n);
                }
                n->parent->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                cf_rcrb_rotate_right(tree, n->parent->parent);
            }
        } else {
            s = n->parent->parent->left;
            if (CF_RCRB_RED == s->color) {
                n->parent->color = CF_RCRB_BLACK;
                s->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    cf_rcrb_rotate_right(tree, n);
                }
                n->parent->color = CF_RCRB_BLACK;
                n->parent->parent->color = CF_RCRB_RED;
                cf_rcrb_rotate_left(tree, n->parent->parent);
            }
        }
    }
    tree->root->left->color = CF_RCRB_BLACK;
	tree->elements++;
	
	pthread_mutex_unlock(&tree->lock);

    return(0);
}




/* cf_rcrb_successor
 * Find the successor to a given node */
cf_rcrb_node *
cf_rcrb_successor(cf_rcrb_tree *tree, cf_rcrb_node *n)
{
    cf_rcrb_node *s;

    if (tree->sentinel != (s = n->right)) { /* Assignment intentional */
        while (tree->sentinel != s->left)
            s = s->left;
        return(s);
    } else {
        s = n->parent;
        while (n == s->right) {
            n = s;
            s = s->parent;
        }

        if (tree->root == s)
            return(tree->sentinel);

        return(s);
    }
}


/* cf_rcrb_deleterebalance
 * Rebalance a red-black tree after removing a node */
void
cf_rcrb_deleterebalance(cf_rcrb_tree *tree, cf_rcrb_node *r)
{
    cf_rcrb_node *s;

    while ((CF_RCRB_BLACK == r->color) && (tree->root->left != r)) {
        if (r == r->parent->left) {
            s = r->parent->right;
            if (CF_RCRB_RED == s->color) {
                s->color = CF_RCRB_BLACK;
                r->parent->color = CF_RCRB_RED;
                cf_rcrb_rotate_left(tree, r->parent);
                s = r->parent->right;
            }

            if ((CF_RCRB_RED != s->right->color) && (CF_RCRB_RED != s->left->color)) {
                s->color = CF_RCRB_RED;
                r = r->parent;
            } else {
                if (CF_RCRB_RED != s->right->color) {
                    s->left->color = CF_RCRB_BLACK;
                    s->color = CF_RCRB_RED;
                    cf_rcrb_rotate_right(tree, s);
                    s = r->parent->right;
                }
                s->color = r->parent->color;
                r->parent->color = CF_RCRB_BLACK;
                s->right->color = CF_RCRB_BLACK;
                cf_rcrb_rotate_left(tree, r->parent);
                r = tree->root->left;
            }
        } else {
            /* This is a mirror image of the code above */
            s = r->parent->left;
            if (CF_RCRB_RED == s->color) {
                s->color = CF_RCRB_BLACK;
                r->parent->color = CF_RCRB_RED;
                cf_rcrb_rotate_right(tree, r->parent);
                s = r->parent->left;
            }

            if ((CF_RCRB_RED != s->right->color) && (CF_RCRB_RED != s->left->color)) {
                s->color = CF_RCRB_RED;
                r = r->parent;
            } else {
                if (CF_RCRB_RED != s->left->color) {
                    s->right->color = CF_RCRB_BLACK;
                    s->color = CF_RCRB_RED;
                    cf_rcrb_rotate_left(tree, s);
                    s = r->parent->left;
                }
                s->color = r->parent->color;
                r->parent->color = CF_RCRB_BLACK;
                s->left->color = CF_RCRB_BLACK;
                cf_rcrb_rotate_right(tree, r->parent);
                r = tree->root->left;
            }
        }
    }
    r->color = CF_RCRB_BLACK;

    return;
}


/* cf_rcrb_search_lockless
 * Perform a lockless search for a node in a red-black tree */
cf_rcrb_node *
cf_rcrb_search_lockless(cf_rcrb_tree *tree, cf_digest dkey)
{
    cf_rcrb_node *r = tree->root->left;
    cf_rcrb_node *s = NULL;
    int c;

	cf_debug(CF_RB,"search: key %"PRIx64" sentinal %p",*(uint64_t *)&dkey, tree->sentinel);
	
    /* If there are no entries in the tree, we're done */
    if (r == tree->sentinel)
        goto miss;

    s = r;
    while (s != tree->sentinel) {
		
//		cf_debug(CF_RB,"  at %p: key %"PRIx64": right %p left %p",s,*(uint64_t *)&s->key,s->right,s->left);

        c = cf_digest_compare(&dkey, &s->key);
        if (c)
            s = (c > 0) ? s->left : s->right;
        else
            return(s);
    }

    /* No matches found */
miss: 
    return(NULL);
}


/* cf_rcrb_search
 * Search a red-black tree for a node with a particular key */
void *
cf_rcrb_search(cf_rcrb_tree *tree, cf_digest key)
{
    void *v = 0;

    cf_info(CF_RB, " bad use: should not search without taking value lock");
    
    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);

    cf_rcrb_node *n = cf_rcrb_search_lockless(tree, key);
    if (n) {
    	v = n->value;
    	cf_rc_reserve(v);
    }

    /* Unlock the tree */
    pthread_mutex_unlock(&tree->lock);

    return(v);
}




/* cf_rcrb_delete
 * Remove a node from a red-black tree, 
 * returning 0 or any return value from  the provided value destructor function
 * return value:
 *   0 means success
 *   -1 means internal failure
 *   -2 means value not found
 */
int
cf_rcrb_delete(cf_rcrb_tree *tree, cf_digest key)
{
    cf_rcrb_node *r, *s, *t;
	int rv = 0;

    /* Lock the tree */
    if (0 != pthread_mutex_lock(&tree->lock)) {
		cf_warning(CF_RB, "unable to acquire tree lock: %s", cf_strerror(errno));
		return(-1);
	}

    /* Find a node with the matching key; if none exists, eject immediately */
    if (NULL == (r = cf_rcrb_search_lockless(tree, key))) {
		rv = -2;
        goto release;
	}

    s = ((tree->sentinel == r->left) || (tree->sentinel == r->right)) ? r : cf_rcrb_successor(tree, r);
    t = (tree->sentinel == s->left) ? s->right : s->left;
    if (tree->root == (t->parent = s->parent)) /* Assignment OK */
        tree->root->left = t;
    else {
        if (s == s->parent->left)
            s->parent->left = t;
        else
            s->parent->right = t;
    }
    
    /* s is the node to splice out, and t is its child */
    if (s != r) {

        if (CF_RCRB_BLACK == s->color)
            cf_rcrb_deleterebalance(tree, t);
        
        /* Reassign pointers and coloration */
        s->left = r->left;
        s->right = r->right;
        s->parent = r->parent;
        s->color = r->color;
        r->left->parent = r->right->parent = s;

        if (r == r->parent->left)
            r->parent->left = s;
        else
            r->parent->right = s;

		/* Consume the node */
		if (0 == cf_rc_release(r->value)) {
			tree->destructor(r->value, tree->destructor_udata);
		}

        free(r);
        
    } else {

        // I don't understand why this has to be here -b
        if (CF_RCRB_BLACK == s->color)
            cf_rcrb_deleterebalance(tree, t);

		/* Destroy the node contents */
		if (0 == cf_rc_release(r->value)) {
			tree->destructor(r->value, tree->destructor_udata);
		}

        free(s);
    }
	tree->elements--;

release:
    pthread_mutex_unlock(&tree->lock);

    return(rv);
}


/* rb_create
 * Create a new red-black tree */
cf_rcrb_tree *
cf_rcrb_create(cf_rcrb_value_destructor destructor, void *destructor_udata) {
	
    cf_rcrb_tree *tree;

    /* Allocate memory for the tree and initialize the tree lock */
    if (NULL == (tree = cf_rc_alloc(sizeof(cf_rcrb_tree))))
        return(NULL);

	pthread_mutex_init(&tree->lock, NULL);

    /* Allocate memory for the sentinel; note that it's pointers are all set
     * to itself */
    if (NULL == (tree->sentinel = (cf_rcrb_node *)calloc(1, sizeof(cf_rcrb_node)))) {
        free(tree);
        return(NULL);
    }
    tree->sentinel->parent = tree->sentinel->left = tree->sentinel->right = tree->sentinel;
    tree->sentinel->color = CF_RCRB_BLACK;

    /* Allocate memory for the root node, and set things up */
    if (NULL == (tree->root = (cf_rcrb_node *)calloc(1, sizeof(cf_rcrb_node)))) {
        free(tree->sentinel);
        free(tree);
        return(NULL);
    }
    tree->root->parent = tree->root->left = tree->root->right = tree->sentinel;
    tree->root->color = CF_RCRB_BLACK;

    tree->destructor = destructor;
    tree->destructor_udata = destructor_udata;
    
	tree->elements = 0;

    /* Return a pointer to the new tree */
    return(tree);
}


/* cf_rcrb_purge
 * Purge a node and, recursively, its children, from a red-black tree */
void
cf_rcrb_purge(cf_rcrb_tree *tree, cf_rcrb_node *r)
{
    /* Don't purge the sentinel */
    if (r == tree->sentinel)
        return;

    /* Purge the children */
    cf_rcrb_purge(tree, r->left);
    cf_rcrb_purge(tree, r->right);

    if (0 == cf_rc_release(r->value)) {
    	tree->destructor(r->value, tree->destructor_udata);
    }
    
	// debug thing
	// memset(r, 0xff, sizeof(cf_rcrb_node));
    free(r);

    return;
}

uint32_t
cf_rcrb_size(cf_rcrb_tree *tree)
{
	uint32_t	sz;
	pthread_mutex_lock(&tree->lock);
	sz = tree->elements;
	pthread_mutex_unlock(&tree->lock);
	return(sz);
}

typedef struct {
	cf_digest 	key;
	void		*value;
} cf_rcrb_value;

typedef struct {
	uint alloc_sz;
	uint pos;
	cf_rcrb_value values[];
} cf_rcrb_value_array;


/*
** call a function on all the nodes in the tree
*/
void
cf_rcrb_reduce_traverse( cf_rcrb_tree *tree, cf_rcrb_node *r, cf_rcrb_node *sentinel, cf_rcrb_reduce_fn cb, void *udata)
{
	cf_rcrb_value_array *v_a = (cf_rcrb_value_array *) udata;
	
	if (v_a->pos >= v_a->alloc_sz)	return;
	
	if (r->value) {
		cf_rc_reserve(r->value);
		v_a->values[v_a->pos].value = r->value;
		v_a->values[v_a->pos].key = r->key;
		v_a->pos++;
    }

	if (r->left != sentinel)		
		cf_rcrb_reduce_traverse(tree, r->left, sentinel, cb, udata);
	
	if (r->right != sentinel)
		cf_rcrb_reduce_traverse(tree, r->right, sentinel, cb, udata);
	
}


void
cf_rcrb_reduce(cf_rcrb_tree *tree, cf_rcrb_reduce_fn cb, void *udata)
{
    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);
	
    // I heart stack allocation. I should probably make this an if and use malloc
    // if it's large
    uint	sz = sizeof( cf_rcrb_value_array ) + ( sizeof(cf_rcrb_value) * tree->elements);
    cf_rcrb_value_array *v_a;
    uint8_t buf[128 * 1024];
    
    if (sz > 128 * 1024) {
    	v_a = malloc(sz);
    	if (!v_a)	return;
    }
    else
    	v_a = (cf_rcrb_value_array *) buf;
	
    v_a->alloc_sz = tree->elements;
    v_a->pos = 0;
    
	if ( (tree->root) && 
		 (tree->root->left) && 
		 (tree->root->left != tree->sentinel) )
		cf_rcrb_reduce_traverse(tree, tree->root->left, tree->sentinel, cb, v_a);

	pthread_mutex_unlock(&tree->lock);
	
	for (uint i=0 ; i<v_a->pos ; i++) 
		cb ( & (v_a->values[i].key), v_a->values[i].value  , udata);

	if (v_a != (cf_rcrb_value_array *) buf)	free(v_a);
	
    return;
	
}


/* cf_rcrb_release
 * Destroy a red-black tree; return 0 if the tree was destroyed or 1
 * otherwise */
int
cf_rcrb_release(cf_rcrb_tree *tree, void *destructor_udata)
{
	if (0 != cf_rc_release(tree))
		return(1);

	/* Purge the tree and all it's ilk */
	pthread_mutex_lock(&tree->lock);
    cf_rcrb_purge(tree, tree->root->left);

    /* Release the tree's memory */
    free(tree->root);
    free(tree->sentinel);
	pthread_mutex_unlock(&tree->lock);
	memset(tree, 0, sizeof(cf_rcrb_tree)); // a little debug
    cf_rc_free(tree);

    return(0);
}
