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
cf_rb_insert(cf_rb_tree *tree, cf_digest key, void *value)
{
    cf_rb_node *n, *s, *t, *u;
	cf_mcslock_qnode ql;

    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rb_node *)calloc(1, sizeof(cf_rb_node))))
        return(NULL);
	n->key = key;
	n->value = value;
    n->left = n->right = tree->sentinel;
    n->color = CF_RB_RED;
	if (0 != pthread_mutex_init(&n->VALUE_LOCK, NULL)) {
        free(n);
        return(NULL);
    }
    u = n;

    /* Lock the tree */
	cf_mcslock_lock(&tree->lock, &ql);

    /* Insert the node directly into the tree, via the typical method of
     * binary tree insertion */
    n->left = n->right = tree->sentinel;
    s = tree->root;
    t = tree->root->left;
    while (t != tree->sentinel) {
        s = t;
        t = (0 <= memcmp(n->key.digest, t->key.digest, CF_DIGEST_KEY_SZ)) ? t->left : t->right;
    }
    n->parent = s;

    /* If the node already exists, stop a double-insertion */
    if ((s != tree->root) && (0 == memcmp(n->key.digest, s->key.digest, CF_DIGEST_KEY_SZ))) {
        free(n);
		cf_mcslock_unlock(&tree->lock, &ql);
        return(NULL);
    }

    /* Insert the node */
    if ((s == tree->root) || (0 < memcmp(n->key.digest, s->key.digest, CF_DIGEST_KEY_SZ)))
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
                n->parent->parent->color = CF_RB_BLACK;
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

	cf_mcslock_unlock(&tree->lock, &ql);
    return(u);
}


/* cf_rb_insert_vlock
 * Insert a node with a given key into a red-black tree and acquire the
 * value lock */
cf_rb_node *
cf_rb_insert_vlock(cf_rb_tree *tree, cf_digest key, void *value, pthread_mutex_t **vlock)
{
    cf_rb_node *n, *s, *t, *u;
	cf_mcslock_qnode ql;

    /* We'll update this later if we succeed */
    *vlock = NULL;

    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rb_node *)calloc(1, sizeof(cf_rb_node)))) {
		D(" malloc failed ");
        return(NULL);
	}
	n->key = key;
	n->value = value;
    n->left = n->right = tree->sentinel;
    n->color = CF_RB_RED;
	if (0 != pthread_mutex_init(&n->VALUE_LOCK, NULL)) {
		D(" mutex init failed ");
        free(n);
        return(NULL);
    }
    u = n;
	
    /* Lock the tree */
	cf_mcslock_lock(&tree->lock, &ql);

    /* Insert the node directly into the tree, via the typical method of
     * binary tree insertion */
    n->left = n->right = tree->sentinel;
    s = tree->root;
    t = tree->root->left;
    while (t != tree->sentinel) {
        s = t;
        t = (0 <= memcmp(n->key.digest, t->key.digest, CF_DIGEST_KEY_SZ)) ? t->left : t->right;
    }
    n->parent = s;

    /* If the node already exists, stop a double-insertion */
    if ((s != tree->root) && (0 == memcmp(n->key.digest, s->key.digest, CF_DIGEST_KEY_SZ))) {
		pthread_mutex_destroy(&n->VALUE_LOCK);
        free(n);
		cf_mcslock_unlock(&tree->lock, &ql);
        return(NULL);
    }

    /* Insert the node */
    if ((s == tree->root) || (0 < memcmp(n->key.digest, s->key.digest, CF_DIGEST_KEY_SZ)))
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
                n->parent->parent->color = CF_RB_BLACK;
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
	if (0 != pthread_mutex_lock(&n->VALUE_LOCK)) {
		D(" what? can't lock mutex? So BONED!");
	}

	cf_mcslock_unlock(&tree->lock, &ql);

    *vlock = &n->VALUE_LOCK;

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
cf_rb_search_lockless(cf_rb_tree *tree, cf_digest dkey)
{
    cf_rb_node *r = tree->root->left;
    cf_rb_node *s = NULL;
    int c;

    /* If there are no entries in the tree, we're done */
    if (r == tree->sentinel)
        goto miss;

    s = r;
    while (s != tree->sentinel) {
        c = memcmp(dkey.digest, s->key.digest, CF_DIGEST_KEY_SZ);
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
cf_rb_search(cf_rb_tree *tree, cf_digest key)
{
    cf_rb_node *r;
	cf_mcslock_qnode ql;

    /* Lock the tree */
	cf_mcslock_lock(&tree->lock, &ql);

    r = cf_rb_search_lockless(tree, key);

    /* Unlock the tree */
	cf_mcslock_unlock(&tree->lock, &ql);

    return(r);
}


/* cf_rb_search_vlock
 * Search a red-black tree for a node with a particular key and acquire the
 * value lock */
cf_rb_node *
cf_rb_search_vlock(cf_rb_tree *tree, cf_digest key, pthread_mutex_t **vlock)
{
    cf_rb_node *r;
	cf_mcslock_qnode ql;

    /* Lock the tree */
	cf_mcslock_lock(&tree->lock, &ql);

    r = cf_rb_search_lockless(tree, key);

    /* Acquire the value lock */
    if (r) {
        if (0 != pthread_mutex_lock(&r->VALUE_LOCK))
            perror("rb_search_vlock: failed to acquire vlock");
        *vlock = &r->VALUE_LOCK;
    }

    /* Unlock the tree */
	cf_mcslock_unlock(&tree->lock, &ql);

    return(r);
}


/* cf_rb_delete
 * Remove a node from a red-black tree */
void
cf_rb_delete(cf_rb_tree *tree, cf_digest key)
{
    cf_rb_node *r, *s, *t;
	cf_mcslock_qnode ql;

    /* Lock the tree */
	cf_mcslock_lock(&tree->lock, &ql);

    /* Find a node with the matching key; if none exists, eject immediately */
    if (NULL == (r = cf_rb_search_lockless(tree, key)))
        goto release;

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

        /* Free memory for the node contents */
		/* this is an as_record, can't simply be freed */
        // free(r->value);

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
        free(r);
    } else {
		// free memory of node contents - this is an as_record,
		// can't simply be freed
        // free(s->value);
        if (CF_RB_BLACK == s->color)
            cf_rb_deleterebalance(tree, t);
        free(s);
    }
	tree->elements--;
	
release:
	cf_mcslock_unlock(&tree->lock, &ql);

    return;
}


/* rb_create
 * Create a new red-black tree */
cf_rb_tree *
cf_rb_create(cf_rb_value_destructor destructor) {
    cf_rb_tree *tree;

    /* Allocate memory for the tree and initialize the tree lock */
    if (NULL == (tree = (cf_rb_tree *)calloc(1, sizeof(cf_rb_tree))))
        return(NULL);
    
	cf_mcslock_init(&tree->lock);

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

    /* Return a pointer to the new tree */
    return(tree);
}


/* cf_rb_purge
 * Purge a node and, recursively, its children, from a red-black tree */
void
cf_rb_purge(cf_rb_tree *tree, cf_rb_node *r)
{
    /* Don't purge the sentinel */
    if (r == tree->sentinel)
        return;

    /* Purge the children */
    cf_rb_purge(tree, r->left);
    cf_rb_purge(tree, r->right);

    /* Release this node's memory */
    pthread_mutex_lock(&r->VALUE_LOCK);
    /* FIXME We ought to handle this by passing in a destructor callback... */
    if (tree->destructor)
        tree->destructor(r->value);
    pthread_mutex_unlock(&r->VALUE_LOCK);
    pthread_mutex_destroy(&r->VALUE_LOCK);
    free(r);

    return;
}

/*
** call a function on all the nodes in the tree
*/
void
cf_rb_reduce_traverse(  cf_rb_node *r, cf_rb_node *sentinel, cf_rb_reduce_fn cb, void *udata)
{
	if (r->value) {
        pthread_mutex_lock(&r->VALUE_LOCK);
		(cb) (r->key, r->value, udata);
        pthread_mutex_unlock(&r->VALUE_LOCK);
    }

	if (r->left != sentinel)		
		cf_rb_reduce_traverse(r->left, sentinel, cb, udata);
	
	if (r->right != sentinel)
		cf_rb_reduce_traverse(r->right, sentinel, cb, udata);
	
}


void
cf_rb_reduce(cf_rb_tree *tree, cf_rb_reduce_fn cb, void *udata)
{
	cf_mcslock_qnode ql;

    /* Lock the tree */
	cf_mcslock_lock(&tree->lock, &ql);
	
	if ( (tree->root) && 
		 (tree->root->left) && 
		 (tree->root->left != tree->sentinel) )
		cf_rb_reduce_traverse(tree->root->left, tree->sentinel, cb, udata);

	cf_mcslock_unlock(&tree->lock, &ql);
    return;
	
}

/* cf_rb_destroy
 * Destroy a red-black tree */
void
cf_rb_destroy(cf_rb_tree *tree)
{
    /* Purge the root and all its ilk */
    cf_rb_purge(tree, tree->root->left);

    /* Release the tree's memory */
    free(tree->root);
    free(tree->sentinel);
    free(tree);

    return;
}
