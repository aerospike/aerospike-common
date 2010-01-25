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


/* cf_rb_rotate_left
 * Rotate a tree left */
void
cf_rb_rotate_left(cf_rb_tree *tree, cf_rb_node *r)
{
    cf_rb_node *s = r->right;

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


/* cf_rb_rotate_right
 * Rotate a tree right */
void
cf_rb_rotate_right(cf_rb_tree *tree, cf_rb_node *r)
{
    cf_rb_node *s = r->left;

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


/* cf_rb_insert
 * Insert a node with a given key into a red-black tree */
cf_rb_node *
cf_rb_insert(cf_rb_tree *tree, cf_digest *key, void *value)
{
    cf_rb_node *n, *s, *t, *u;

    /* Allocate memory for the new node and set the node parameters */
	// this could be done later, but doing the malloc ahead of the tree lock
	// increases parallelism and decreases lock hold times
    if (NULL == (n = (cf_rb_node *)malloc(sizeof(cf_rb_node))))
        return(NULL);
    n->color = CF_RB_RED;
	n->key = *key;
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
		int c = cf_digest_compare(key, &t->key);
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
        return(NULL);
    }

    /* Insert the node */
    if ((s == tree->root) || (0 < cf_digest_compare(&n->key, &s->key)))
        s->left = n;
    else
        s->right = n;

    /* Rebalance the tree */
    while (CF_RB_RED == n->parent->color) {
        if (n->parent == n->parent->parent->left) {
            s = n->parent->parent->right;
            if (CF_RB_RED == s->color) {
                n->parent->color = CF_RB_BLACK;
                s->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    cf_rb_rotate_left(tree, n);
                }
                n->parent->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                cf_rb_rotate_right(tree, n->parent->parent);
            }
        } else {
            s = n->parent->parent->left;
            if (CF_RB_RED == s->color) {
                n->parent->color = CF_RB_BLACK;
                s->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    cf_rb_rotate_right(tree, n);
                }
                n->parent->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                cf_rb_rotate_left(tree, n->parent->parent);
            }
        }
    }
    tree->root->left->color = CF_RB_BLACK;
	tree->elements++;

	pthread_mutex_unlock(&tree->lock);
    return(u);
}


/* cf_rb_insert_vlock
 * Insert a node with a given key into a red-black tree and acquire the
 * value lock */
cf_rb_node *
cf_rb_insert_vlock(cf_rb_tree *tree, cf_digest *key, pthread_mutex_t **vlock)
{
    cf_rb_node *n, *s, *t, *u;

    /* We'll update this later if we succeed */
    *vlock = NULL;

    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rb_node *)malloc(sizeof(cf_rb_node)))) {
		cf_debug(CF_RB," malloc failed ");
        return(NULL);
	}
	n->key = *key;
	n->value = 0;
//    n->left = n->right = tree->sentinel;
    n->color = CF_RB_RED;
    u = n;
	
    /* Lock the tree */
	pthread_mutex_lock(&tree->lock);

    /* Insert the node directly into the tree, via the typical method of
     * binary tree insertion */
    n->left = n->right = tree->sentinel;
    s = tree->root;
    t = tree->root->left;
    while (t != tree->sentinel) {
        s = t;
		int c = cf_digest_compare(&n->key, &t->key);
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
        return(NULL);
    }

    /* Insert the node */
    if ((s == tree->root) || (0 < cf_digest_compare(&n->key, &s->key)))
        s->left = n;
    else
        s->right = n;

    /* Rebalance the tree */
    while (CF_RB_RED == n->parent->color) {
        if (n->parent == n->parent->parent->left) {
            s = n->parent->parent->right;
            if (CF_RB_RED == s->color) {
                n->parent->color = CF_RB_BLACK;
                s->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    cf_rb_rotate_left(tree, n);
                }
                n->parent->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                cf_rb_rotate_right(tree, n->parent->parent);
            }
        } else {
            s = n->parent->parent->left;
            if (CF_RB_RED == s->color) {
                n->parent->color = CF_RB_BLACK;
                s->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    cf_rb_rotate_right(tree, n);
                }
                n->parent->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                cf_rb_rotate_left(tree, n->parent->parent);
            }
        }
    }
    tree->root->left->color = CF_RB_BLACK;
	tree->elements++;

	// TODO: Bug. This error case can't really be handled without removing the element
	// from the tree again, we're handing back an unlocked lock and shit.
	olock_vlock( tree->value_locks, &u->key, vlock);

	pthread_mutex_unlock(&tree->lock);


    return(u);
}




/* cf_rb_insert2
 * Insert a node with a given key into a red-black tree and acquire the
 * value lock. If the key already exists, return the node and don't set the value */
cf_rb_node *
cf_rb_get_insert_vlock(cf_rb_tree *tree, cf_digest *key, pthread_mutex_t **vlock)
{
    cf_rb_node *n, *s, *t, *u;
    *vlock = NULL;

    /* Lock the tree */
	pthread_mutex_lock(&tree->lock);

    /* Insert the node directly into the tree, via the typical method of
     * binary tree insertion */
    s = tree->root;
    t = tree->root->left;
	cf_debug(CF_RB,"get-insert: key %"PRIx64" sentinal %p",*(uint64_t *)key, tree->sentinel);

    while (t != tree->sentinel) {
        s = t;
		cf_debug(CF_RB,"  at %p: key %"PRIx64": right %p left %p",t,*(uint64_t *)&t->key,t->right,t->left);

		int c = cf_digest_compare(key, &t->key);
        if (c)
            t = (c > 0) ? t->left : t->right;
        else
			break;
    }

    /* If the node already exists, stop a double-insertion */
    if ((s != tree->root) && (0 == cf_digest_compare(key, &s->key))) {
		
		olock_vlock( tree->value_locks, &s->key, vlock);
		pthread_mutex_unlock(&tree->lock);
        return(s);
    }

	cf_debug(CF_RB,"get-insert: not found");
	
    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rb_node *)malloc(sizeof(cf_rb_node)))) {
		cf_debug(CF_RB," malloc failed ");
        return(NULL);
	}
	n->key = *key;
    n->left = n->right = tree->sentinel;
    n->color = CF_RB_RED;
    n->parent = s;
    u = n;

    /* Insert the node */
    if ((s == tree->root) || (0 < cf_digest_compare(&n->key, &s->key)))
        s->left = n;
    else
        s->right = n;

    /* Rebalance the tree */
    while (CF_RB_RED == n->parent->color) {
        if (n->parent == n->parent->parent->left) {
            s = n->parent->parent->right;
            if (CF_RB_RED == s->color) {
                n->parent->color = CF_RB_BLACK;
                s->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    cf_rb_rotate_left(tree, n);
                }
                n->parent->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                cf_rb_rotate_right(tree, n->parent->parent);
            }
        } else {
            s = n->parent->parent->left;
            if (CF_RB_RED == s->color) {
                n->parent->color = CF_RB_BLACK;
                s->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    cf_rb_rotate_right(tree, n);
                }
                n->parent->color = CF_RB_BLACK;
                n->parent->parent->color = CF_RB_RED;
                cf_rb_rotate_left(tree, n->parent->parent);
            }
        }
    }
    tree->root->left->color = CF_RB_BLACK;
	tree->elements++;

	olock_vlock( tree->value_locks, &u->key, vlock);

	pthread_mutex_unlock(&tree->lock);

    return(u);
}


/* cf_rb_successor
 * Find the successor to a given node */
cf_rb_node *
cf_rb_successor(cf_rb_tree *tree, cf_rb_node *n)
{
    cf_rb_node *s;

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


/* cf_rb_deleterebalance
 * Rebalance a red-black tree after removing a node */
void
cf_rb_deleterebalance(cf_rb_tree *tree, cf_rb_node *r)
{
    cf_rb_node *s;

    while ((CF_RB_BLACK == r->color) && (tree->root->left != r)) {
        if (r == r->parent->left) {
            s = r->parent->right;
            if (CF_RB_RED == s->color) {
                s->color = CF_RB_BLACK;
                r->parent->color = CF_RB_RED;
                cf_rb_rotate_left(tree, r->parent);
                s = r->parent->right;
            }

            if ((CF_RB_RED != s->right->color) && (CF_RB_RED != s->left->color)) {
                s->color = CF_RB_RED;
                r = r->parent;
            } else {
                if (CF_RB_RED != s->right->color) {
                    s->left->color = CF_RB_BLACK;
                    s->color = CF_RB_RED;
                    cf_rb_rotate_right(tree, s);
                    s = r->parent->right;
                }
                s->color = r->parent->color;
                r->parent->color = CF_RB_BLACK;
                s->right->color = CF_RB_BLACK;
                cf_rb_rotate_left(tree, r->parent);
                r = tree->root->left;
            }
        } else {
            /* This is a mirror image of the code above */
            s = r->parent->left;
            if (CF_RB_RED == s->color) {
                s->color = CF_RB_BLACK;
                r->parent->color = CF_RB_RED;
                cf_rb_rotate_right(tree, r->parent);
                s = r->parent->left;
            }

            if ((CF_RB_RED != s->right->color) && (CF_RB_RED != s->left->color)) {
                s->color = CF_RB_RED;
                r = r->parent;
            } else {
                if (CF_RB_RED != s->left->color) {
                    s->right->color = CF_RB_BLACK;
                    s->color = CF_RB_RED;
                    cf_rb_rotate_left(tree, s);
                    s = r->parent->left;
                }
                s->color = r->parent->color;
                r->parent->color = CF_RB_BLACK;
                s->left->color = CF_RB_BLACK;
                cf_rb_rotate_right(tree, r->parent);
                r = tree->root->left;
            }
        }
    }
    r->color = CF_RB_BLACK;

    return;
}


/* cf_rb_search_lockless
 * Perform a lockless search for a node in a red-black tree */
cf_rb_node *
cf_rb_search_lockless(cf_rb_tree *tree, cf_digest *key)
{
    cf_rb_node *r = tree->root->left;
    cf_rb_node *s = NULL;
    int c;

	cf_debug(CF_RB,"search: key %"PRIx64" sentinal %p",*(uint64_t *)key, tree->sentinel);
	
    /* If there are no entries in the tree, we're done */
    if (r == tree->sentinel)
        goto miss;

    s = r;
    while (s != tree->sentinel) {
		
//		cf_debug(CF_RB,"  at %p: key %"PRIx64": right %p left %p",s,*(uint64_t *)&s->key,s->right,s->left);

        c = cf_digest_compare(key, &s->key);
        if (c)
            s = (c > 0) ? s->left : s->right;
        else
            return(s);
    }

    /* No matches found */
miss: 
    return(NULL);
}


/* cf_rb_search
 * Search a red-black tree for a node with a particular key */
cf_rb_node *
cf_rb_search(cf_rb_tree *tree, cf_digest *key)
{
    cf_rb_node *r;

    cf_info(CF_RB, " bad use: should not search without taking value lock");
    
    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);

    r = cf_rb_search_lockless(tree, key);

    /* Unlock the tree */
    pthread_mutex_unlock(&tree->lock);

    return(r);
}


/* cf_rb_search_vlock
 * Search a red-black tree for a node with a particular key and acquire the
 * value lock */
cf_rb_node *
cf_rb_search_vlock(cf_rb_tree *tree, cf_digest *key, pthread_mutex_t **vlock)
{
    cf_rb_node *r;

    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);

    r = cf_rb_search_lockless(tree, key);

    /* Acquire the value lock */
    if (r) {
		olock_vlock( tree->value_locks, &r->key, vlock);
    }

    /* Unlock the tree */
    pthread_mutex_unlock(&tree->lock);

    return(r);
}




/* cf_rb_delete
 * Remove a node from a red-black tree, 
 * returning 0 or any return value from  the provided value destructor function
 * return value:
 *   0 means success
 *   -1 means internal failure
 *   -2 means value not found
 */
int
cf_rb_delete(cf_rb_tree *tree, cf_digest *key, void *destructor_udata)
{
    cf_rb_node *r, *s, *t;
	int rv = 0;

    /* Lock the tree */
    if (0 != pthread_mutex_lock(&tree->lock)) {
		cf_warning(CF_RB, "unable to acquire tree lock: %s", cf_strerror(errno));
		return(-1);
	}

    /* Find a node with the matching key; if none exists, eject immediately */
    if (NULL == (r = cf_rb_search_lockless(tree, key))) {
		rv = -2;
        goto release;
	}

	/* Hold the value lock */
    pthread_mutex_t *vlock = 0;
	olock_vlock( tree->value_locks, &r->key, &vlock);

    s = ((tree->sentinel == r->left) || (tree->sentinel == r->right)) ? r : cf_rb_successor(tree, r);
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

        if (CF_RB_BLACK == s->color)
            cf_rb_deleterebalance(tree, t);
        
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
		if (tree->destructor)
	        tree->destructor(r->value, destructor_udata);
		else
			free(r->value);

		// todoo
        pthread_mutex_unlock(vlock);
        
        free(r);
    } else {

		/* Destroy the node contents */
		if (tree->destructor)
	        tree->destructor(s->value, destructor_udata);
		else
			free(s->value);
        
        pthread_mutex_unlock(vlock);

        // I don't understand why this has to be here -b
        if (CF_RB_BLACK == s->color)
            cf_rb_deleterebalance(tree, t);

        free(s);
    }
	tree->elements--;

release:
    pthread_mutex_unlock(&tree->lock);

    return(rv);
}


/* rb_create
 * Create a new red-black tree */
cf_rb_tree *
cf_rb_create(cf_rb_value_destructor destructor) {
    cf_rb_tree *tree;

    /* Allocate memory for the tree and initialize the tree lock */
    if (NULL == (tree = cf_rc_alloc(sizeof(cf_rb_tree))))
        return(NULL);

	tree->value_locks = olock_create( 16, true );

	pthread_mutex_init(&tree->lock, NULL);

    /* Allocate memory for the sentinel; note that it's pointers are all set
     * to itself */
    if (NULL == (tree->sentinel = (cf_rb_node *)calloc(1, sizeof(cf_rb_node)))) {
        free(tree);
        return(NULL);
    }
    tree->sentinel->parent = tree->sentinel->left = tree->sentinel->right = tree->sentinel;
    tree->sentinel->color = CF_RB_BLACK;

    /* Allocate memory for the root node, and set things up */
    if (NULL == (tree->root = (cf_rb_node *)calloc(1, sizeof(cf_rb_node)))) {
        free(tree->sentinel);
        free(tree);
        return(NULL);
    }
    tree->root->parent = tree->root->left = tree->root->right = tree->sentinel;
    tree->root->color = CF_RB_BLACK;

    tree->destructor = destructor;
	tree->elements = 0;

    /* Return a pointer to the new tree */
    return(tree);
}


/* cf_rb_purge
 * Purge a node and, recursively, its children, from a red-black tree */
void
cf_rb_purge(cf_rb_tree *tree, cf_rb_node *r, void *destructor_udata)
{
    /* Don't purge the sentinel */
    if (r == tree->sentinel)
        return;

    /* Purge the children */
    cf_rb_purge(tree, r->left, destructor_udata);
    cf_rb_purge(tree, r->right, destructor_udata);

    /* Release this node's memory (I don't think this lock is necessary - b */
	olock_lock(tree->value_locks, &r->key);
	if (tree->destructor)
        (void)tree->destructor(r->value, destructor_udata);
	olock_unlock(tree->value_locks, &r->key);
	// debug thing
	// memset(r, 0xff, sizeof(cf_rb_node));
    free(r);

    return;
}

uint32_t
cf_rb_size(cf_rb_tree *tree)
{
	uint32_t	sz;
	pthread_mutex_lock(&tree->lock);
	sz = tree->elements;
	pthread_mutex_unlock(&tree->lock);
	return(sz);
}
/*
** call a function on all the nodes in the tree
*/
void
cf_rb_reduce_traverse( cf_rb_tree *tree, cf_rb_node *r, cf_rb_node *sentinel, cf_rb_reduce_fn cb, void *udata)
{
	if (r->value) {
		olock_lock(tree->value_locks, &r->key);
		(cb) (&r->key, r->value, udata);
		olock_unlock(tree->value_locks, &r->key);
    }

	if (r->left != sentinel)		
		cf_rb_reduce_traverse(tree, r->left, sentinel, cb, udata);
	
	if (r->right != sentinel)
		cf_rb_reduce_traverse(tree, r->right, sentinel, cb, udata);
	
}


void
cf_rb_reduce(cf_rb_tree *tree, cf_rb_reduce_fn cb, void *udata)
{
    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);
	
	if ( (tree->root) && 
		 (tree->root->left) && 
		 (tree->root->left != tree->sentinel) )
		cf_rb_reduce_traverse(tree, tree->root->left, tree->sentinel, cb, udata);

	pthread_mutex_unlock(&tree->lock);
    return;
	
}


/* cf_rb_release
 * Destroy a red-black tree; return 0 if the tree was destroyed or 1
 * otherwise */
int
cf_rb_release(cf_rb_tree *tree, void *destructor_udata)
{
	if (0 != cf_rc_release(tree))
		return(1);

	/* Purge the tree and all it's ilk */
	pthread_mutex_lock(&tree->lock);
    cf_rb_purge(tree, tree->root->left, destructor_udata);

    /* Release the tree's memory */
    free(tree->root);
    free(tree->sentinel);
	pthread_mutex_unlock(&tree->lock);
	memset(tree, 0, sizeof(cf_rb_tree)); // a little debug
    cf_rc_free(tree);

    return(0);
}
