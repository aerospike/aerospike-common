/*
 *  Citrusleaf Foundation
 *  src/rb.c - red-black trees
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include "server/cf.h"

#include <errno.h>	
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>


#define TIMETREE 1

extern void incr_global_record_ref_count(void);
extern void decr_global_record_ref_count(void);
extern void incr_global_tree_count(void);
extern void decr_global_tree_count(void);
extern void incr_err_rcrb_reduce_gt5(void);
extern void incr_err_rcrb_reduce_gt50(void);
extern void incr_err_rcrb_reduce_gt100(void);
extern void incr_err_rcrb_reduce_gt250(void);

#ifdef TIMETREE
void rcrb_count_time(uint64_t start)
{
	int elapsed = cf_getms() - start;

	if (elapsed > 250)  	
		incr_err_rcrb_reduce_gt250();
	else if (elapsed > 100)  	
		incr_err_rcrb_reduce_gt100();
	else if (elapsed > 50)  	
		incr_err_rcrb_reduce_gt50();
	else if (elapsed > 5)  	
		incr_err_rcrb_reduce_gt5();
}
#endif 

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
cf_rcrb_node *
cf_rcrb_insert_vlock(cf_rcrb_tree *tree, cf_digest *key, pthread_mutex_t **vlock)
{
    cf_rcrb_node *n, *s, *t, *u;

#ifdef TIMETREE
    cf_clock now = cf_getms();
#endif    
    
    /* Lock the tree */
	pthread_mutex_lock(&tree->lock);
	*vlock = &tree->lock;

    /* find the place to insert, via the typical method of
     * binary tree insertion */
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

    /* If the node already exists, stop a double-insertion */
    if ((s != tree->root) && (0 == cf_digest_compare(key, &s->key))) {
		pthread_mutex_unlock(&tree->lock);
#ifdef TIMETREE		
		rcrb_count_time(now);
#endif		
        return(0);
    }

    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rcrb_node *)cf_malloc(sizeof(cf_rcrb_node)))) {
    	pthread_mutex_unlock(&tree->lock);
#ifdef TIMETREE    	
		rcrb_count_time(now);
#endif		
        return(0);
    }
    n->color = CF_RCRB_RED;
	n->key = *key;
	n->value = 0;
    n->parent = s;
	n->left = n->right = tree->sentinel;
	
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

#ifdef TIMETREE
	rcrb_count_time(now);
#endif	
	
    return(u);
}




/* cf_rcrb_get_insert
 * Get or insert a node with a given tree into a red-black tree.
 *
 * The purpose of this admittadly strange API is it allows the caller to insert a new value after
 * determinging whether the item existed in the first place.
 * */
cf_rcrb_node *
cf_rcrb_get_insert_vlock(cf_rcrb_tree *tree, cf_digest *key, pthread_mutex_t **vlock)
{
    cf_rcrb_node *n, *s, *t, *u;

#ifdef TIMETREE    
    cf_clock now = cf_getms();
#endif    
    
    /* Lock the tree */
	pthread_mutex_lock(&tree->lock);
	*vlock = &tree->lock;

    /* Insert the node directly into the tree, via the typical method of
     * binary tree insertion */
    s = tree->root;
    t = tree->root->left;
	cf_debug(CF_RB,"get-insert: key %"PRIx64" sentinal %p",*(uint64_t *)key, tree->sentinel);

    while (t != tree->sentinel) {
        s = t;
//		cf_debug(CF_RB,"  at %p: key %"PRIx64": right %p left %p",t,*(uint64_t *)&t->key,t->right,t->left);

		int c = cf_digest_compare(key, &t->key);
        if (c)
            t = (c > 0) ? t->left : t->right;
        else
			break;
    }

    /* If the node already exists, simply return it */
    if ((s != tree->root) && (0 == cf_digest_compare(key, &s->key))) {
#ifdef TIMETREE
		rcrb_count_time(now);
#endif    	
        return(s);
        
    }

//	cf_debug(CF_RB,"get-insert: not found");
	
    /* Allocate memory for the new node and set the node parameters */
    if (NULL == (n = (cf_rcrb_node *)cf_malloc(sizeof(cf_rcrb_node)))) {
		cf_debug(CF_RB," malloc failed ");
        return(NULL);
	}
	n->key = *key;
	n->value = 0;
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

#ifdef TIMETREE
	rcrb_count_time(now);
#endif

    return(u);
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
cf_rcrb_search_lockless(cf_rcrb_tree *tree, cf_digest *key)
{
    cf_rcrb_node *r = tree->root->left;
    cf_rcrb_node *s = NULL;
    int c;

	cf_debug(CF_RB,"search: key %"PRIx64" sentinal %p",*(uint64_t *)&key, tree->sentinel);
	
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


/* cf_rcrb_search
 * Search a red-black tree for a node with a particular key */
void *
cf_rcrb_search(cf_rcrb_tree *tree, cf_digest *key)
{
    void *v = 0;

    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);

    cf_rcrb_node *n = cf_rcrb_search_lockless(tree, key);
    if (n) {
    	v = n->value;
    	cf_rc_reserve(v);
		cf_detail(AS_RECORD, "cf_rcrb_search EXISTING RECORD REFERENCE ACQUIRED:  %p", v);
		incr_global_record_ref_count();
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
cf_rcrb_delete(cf_rcrb_tree *tree, cf_digest *key)
{
    cf_rcrb_node *r, *s, *t;
	int rv = 0;

#ifdef TIMETREE	
	cf_clock now = cf_getms();
#endif	
	
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
		cf_detail(AS_RECORD, "cf_rcrb_delete RECORD REFERENCE RELEASED:  %p", r->value);
		decr_global_record_ref_count();
		if (0 == cf_rc_release(r->value)) {
			tree->destructor(r->value, tree->destructor_udata);
		}

        cf_free(r);
        
    } else {

        // I don't understand why this has to be here -b
        if (CF_RCRB_BLACK == s->color)
            cf_rcrb_deleterebalance(tree, t);

		cf_detail(AS_RECORD, "cf_rcrb_delete RECORD REFERENCE RELEASED:  %p", s->value);
		decr_global_record_ref_count();
		/* Destroy the node contents */
		if (0 == cf_rc_release(s->value)) {
			tree->destructor(s->value, tree->destructor_udata);
		}

        cf_free(s);
    }
	tree->elements--;

release:
    pthread_mutex_unlock(&tree->lock);
#ifdef TIMETREE
	rcrb_count_time(now);
#endif
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
    if (NULL == (tree->sentinel = (cf_rcrb_node *)cf_calloc(1, sizeof(cf_rcrb_node)))) {
        cf_free(tree);
        return(NULL);
    }
    tree->sentinel->parent = tree->sentinel->left = tree->sentinel->right = tree->sentinel;
    tree->sentinel->color = CF_RCRB_BLACK;

    /* Allocate memory for the root node, and set things up */
    if (NULL == (tree->root = (cf_rcrb_node *)cf_calloc(1, sizeof(cf_rcrb_node)))) {
        cf_free(tree->sentinel);
        cf_free(tree);
        return(NULL);
    }
    tree->root->parent = tree->root->left = tree->root->right = tree->sentinel;
    tree->root->color = CF_RCRB_BLACK;

    tree->destructor = destructor;
    tree->destructor_udata = destructor_udata;
    
	tree->elements = 0;

	incr_global_tree_count();
	cf_debug(AS_RECORD, "cf_rcrb_create CREATING TREE :  %p", tree);
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

	cf_detail(AS_RECORD, "cf_rcrb_purge RECORD REFERENCE RELEASED:  %p", r->value);
	decr_global_record_ref_count();
    if (0 == cf_rc_release(r->value)) {
    	tree->destructor(r->value, tree->destructor_udata);
    }
    
	// debug thing
	// memset(r, 0xff, sizeof(cf_rcrb_node));
    cf_free(r);

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
cf_rcrb_reduce_traverse( cf_rcrb_tree *tree, cf_rcrb_node *r, cf_rcrb_node *sentinel, cf_rcrb_value_array *v_a)
{
	
	if (v_a->pos >= v_a->alloc_sz)	return;
	
	if (r->value) {
		cf_rc_reserve(r->value);
		incr_global_record_ref_count();
		cf_detail(AS_RECORD, "cf_rcrb_reduce_traverse EXISTING RECORD REFERENCE ACQUIRED:  %p", r->value);
		v_a->values[v_a->pos].value = r->value;
		v_a->values[v_a->pos].key = r->key;
		v_a->pos++;
    }

	if (r->left != sentinel)		
		cf_rcrb_reduce_traverse(tree, r->left, sentinel, v_a);
	
	if (r->right != sentinel)
		cf_rcrb_reduce_traverse(tree, r->right, sentinel, v_a);
	
}


void
cf_rcrb_reduce(cf_rcrb_tree *tree, cf_rcrb_reduce_fn cb, void *udata)
{
#ifdef TIMETREE
	cf_clock now = cf_getms();
#endif

    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);
    
	if (tree->elements == 0)	{
		pthread_mutex_unlock(&tree->lock);
		return;    
	}
	
    // I heart stack allocation. 
    uint	sz = sizeof( cf_rcrb_value_array ) + ( sizeof(cf_rcrb_value) * tree->elements);
    cf_rcrb_value_array *v_a;
    uint8_t buf[64 * 1024];
    
    if (sz > 64 * 1024) {
    	v_a = cf_malloc(sz);
    	if (!v_a)	return;
    }
    else
    	v_a = (cf_rcrb_value_array *) buf;
	
    v_a->alloc_sz = tree->elements;
    v_a->pos = 0;

    // recursively, fetch all the value pointers into this array, guarenteed in-memory,
    // so we can call the reduce function outside the big lock
	if ( (tree->root) && 
		 (tree->root->left) && 
		 (tree->root->left != tree->sentinel) )
		cf_rcrb_reduce_traverse(tree, tree->root->left, tree->sentinel, v_a);

	pthread_mutex_unlock(&tree->lock);

#ifdef TIMETREE	
	rcrb_count_time(now);
#endif	
	
	for (uint i=0 ; i<v_a->pos ; i++) 
		cb ( & (v_a->values[i].key), v_a->values[i].value  , udata);

	if (v_a != (cf_rcrb_value_array *) buf)	cf_free(v_a);
	
	
	
    return;
	
}

/*
** call a function on all the nodes in the tree
*/
void
cf_rcrb_reduce_sync_traverse( cf_rcrb_tree *tree, cf_rcrb_node *r, cf_rcrb_node *sentinel, cf_rcrb_reduce_fn cb, void *udata)
{
	if (r->value)
		cb ( &r->key, r->value, udata); 

	if (r->left != sentinel)		
		cf_rcrb_reduce_sync_traverse(tree, r->left, sentinel, cb, udata);
	
	if (r->right != sentinel)
		cf_rcrb_reduce_sync_traverse(tree, r->right, sentinel, cb, udata);
}


void
cf_rcrb_reduce_sync(cf_rcrb_tree *tree, cf_rcrb_reduce_fn cb, void *udata)
{
#ifdef TIMETREE
	cf_clock now = cf_getms();
#endif
    /* Lock the tree */
    pthread_mutex_lock(&tree->lock);
	
	if ( (tree->root) && 
		 (tree->root->left) && 
		 (tree->root->left != tree->sentinel) )
		cf_rcrb_reduce_sync_traverse(tree, tree->root->left, tree->sentinel, cb, udata);

	pthread_mutex_unlock(&tree->lock);
#ifdef TIMETREE
	rcrb_count_time(now);
#endif
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
	cf_debug(AS_RECORD, "cf_rcrb_release FREEING TREE :  %p", tree);
	decr_global_tree_count();

	/* Purge the tree and all it's ilk */
	pthread_mutex_lock(&tree->lock);
    cf_rcrb_purge(tree, tree->root->left);

    /* Release the tree's memory */
    cf_free(tree->root);
    cf_free(tree->sentinel);
	pthread_mutex_unlock(&tree->lock);
	memset(tree, 0, sizeof(cf_rcrb_tree)); // a little debug
    cf_rc_free(tree);

    return(0);
}

/*
** UNIT TESTS
** This unit really needs tests
*/



//
// validate various things about a tree. Does it have the right length?
// is it properly colored?
//

int
key_compar (const void *k1, const void *k2)
{
	return ( memcmp(k1, k2, sizeof(cf_digest)) );
}

typedef struct {
	cf_digest key;
	bool	  active;
} test_data;

typedef struct {
	int alloc_sz;
	int sz;
	test_data d[];
} test_data_array;


typedef struct {
	int max_depth;
	int extra_digests;
	test_data_array *data_a;
} cf_rcrb_validate_data;



int
cf_rcrb_validate_traverse( cf_rcrb_node *r, cf_rcrb_node *sentinel, int depth, int color, cf_rcrb_validate_data *vd)
{
	if (depth > vd->max_depth) vd->max_depth = depth;	
	
	if (r->color == CF_RCRB_RED && r->color == color) {
		cf_info(CF_RB, "ILLEGAL COLOR: node %p color %d depth %d, same as parent",r,r->color,depth);
		return(-1);
	}

	test_data_array *da = vd->data_a;	
	if (da->sz >= da->alloc_sz) {
		cf_info(CF_RB, "VALIDATE: more nodes in tree than suspected, can't record node %p",r);
		vd->extra_digests++;
	}
	else {
		da->d[da->sz].key = r->key;
		da->sz++;
	}
	
	int rv = 0;
	
	if (r->left != sentinel)		
		rv = cf_rcrb_validate_traverse(r->left, sentinel, depth + 1, r->color, vd);
	
	if (rv == 0 && (r->right != sentinel))
		rv = cf_rcrb_validate_traverse(r->right, sentinel, depth + 1, r->color, vd);
	
	return(rv);
}



int 
cf_rcrb_validate_lockfree( cf_rcrb_tree *tree )
{

	cf_detail(CF_RB, "starting validate: %d elements",tree->elements);
	
	int rv = 0;
	
	if ( !tree->root) return(0);
	if ( tree->root->left == 0 ) {
		if (tree->elements != 0) {
			cf_info(CF_RB, "supposedly has elements, doesn't ---");
			return(-1);
		}
	}
	if (tree->root->left == tree->sentinel) {
		if (tree->elements != 0) {
			cf_info(CF_RB, "supposedly has elements, doesn't ---");
			return(-1);
		}
	}

	cf_rcrb_validate_data vd;
	
	vd.max_depth = 0;
	vd.extra_digests = 0;
	
	test_data_array *da = cf_malloc(sizeof(test_data_array) + (tree->elements * sizeof(test_data)));
	memset(da, 0,sizeof(test_data_array) + (tree->elements * sizeof(test_data)) );
	da->alloc_sz = tree->elements;
	vd.data_a = da;

	rv = cf_rcrb_validate_traverse(tree->root->left, tree->sentinel, 1, -1, &vd);

	// make sure all the digests are unique and the number of elements matches what is suspected
	if (tree->elements != da->sz) 
		cf_info(CF_RB, "size mismatch: too few in tree: %d elements counted, elements size %d",da->sz,tree->elements);
	if (vd.extra_digests) 
		cf_info(CF_RB, "size mismatch: too many in tree: %d extra more than %d elements",vd.extra_digests,tree->elements);
		
	if (da->sz > 1) {
		qsort(da->d, da->sz, sizeof(cf_digest), key_compar);
	
		// validate no key is seen twice
		for (int i=1; i < da->sz;i++) {
			if (0 == memcmp(&da->d[i-1], &da->d[i], sizeof(cf_digest))) {
				cf_info(CF_RB, "validate: two of same key in tree, PROBLEM");
				rv = -1;
			}
		}
	}	
	
	if (vd.extra_digests)	rv = -1;

	if (rv == 0)
		cf_info(CF_RB,"validate complete: SUCCESS (depth %d)",vd.max_depth);
	else
		cf_info(CF_RB, "validate complete: FAIL %d extrad %d depth %d tree %p",rv,vd.extra_digests,vd.max_depth,tree);

	return(rv);

}	

#include <signal.h>

// 0 is success, -1 is failure
int
cf_rcrb_validate( cf_rcrb_tree *tree)
{
	
	pthread_mutex_lock(&tree->lock);
	
	int rv = cf_rcrb_validate_lockfree(tree);
	
	pthread_mutex_unlock(&tree->lock);

	return(rv);
}	

//
// Allocate a new array of random things
//


test_data_array *
cf_rcrb_test_make_array(int n)
{
	test_data_array *a = cf_malloc(sizeof(test_data_array) + (n * sizeof(test_data)));
	if (!a)	return(0);
	
	for (int i=0;i<n;i++) {
		uint8_t r_buf[40];
		cf_get_rand_buf(r_buf, sizeof(r_buf));
		cf_digest_compute(r_buf, sizeof(r_buf),&(a->d[i].key) );
		a->d[i].active = false;
	}
	a->alloc_sz = n;
	a->sz = n;
	return(a);
}

//
// the application code never uses insert, always get-insert. So just test
// that code path.
//

int
cf_rcrb_test_insert(cf_rcrb_tree *tree, test_data_array *da, int percent)
{
	int n_inserted = 0;
	// int tree_sz = cf_rcrb_size(tree);
	
	for (int i=0;i<da->sz;i++) {
		
		if (da->d[i].active == false) {
		
			if ( cf_get_rand32() % 100 < percent) {
			
				pthread_mutex_t *lock;
				cf_rcrb_node *node = cf_rcrb_get_insert_vlock( tree, &da->d[i].key, &lock);
				if (!node) return(-1);
				if (node->value == 0) {
					if (da->d[i].active == true) {
						cf_warning(CF_RB, "inserting data: ERROR, active but data not found");
						pthread_mutex_unlock(lock);
						return(-1);
					}
					uint8_t *u = cf_rc_alloc( sizeof(cf_digest) );
					memcpy(u, &da->d[i].key, sizeof(cf_digest) );
					node->value = u;
					da->d[i].active = true;
					n_inserted++;
				}
				else { 
					cf_warning(CF_RB, "inserting data: ERROR, not active but data found");
					pthread_mutex_unlock(lock);
					return(-1);
				}
				pthread_mutex_unlock(lock);
			}
			
		}
	}
	
	cf_info(CF_RB, "inserted %d elements into tree, now %d",n_inserted,cf_rcrb_size(tree));
	
	return(0);
}	

//
// consider changing to validate subportions?
//

int
cf_rcrb_test_validate_data(cf_rcrb_tree *tree, test_data_array *da)
{
	int rv = 0;
	for (int i=0; i<da->sz; i++) {
		cf_digest *d = cf_rcrb_search(tree, &da->d[i].key);
		if (d == 0) {
			if (da->d[i].active == true) {
				cf_info(CF_RB, "ERROR: KEY NOT FOUND in validate data");
				return(-1);
			}
		}
		else {
			if (da->d[i].active == false) {
				cf_info(CF_RB, "ERROR: KEY found when was supposed to be deleted!");
				return(-1);
			}
			
			// validate data
			if (0 != memcmp(d, &da->d[i].key, sizeof(cf_digest) ) ) {
				rv = -1;
				cf_info(CF_RB, "KEY HAS WRONG DATA IN VALIDATE DATA");
			}
			// release acquired reference count
			if (0 == cf_rc_release(d)) {
				cf_rc_free(d);
			}
			if (rv != 0)	return(rv);
		}
	}
	return(0);
}

int cf_rcrb_test_delete(cf_rcrb_tree *tree, test_data_array *da, int percent)
{
	int n_deleted = 0;
	int tree_sz = cf_rcrb_size(tree);
	
	for (int i=0 ; i<da->sz ; i++) {
		if (da->d[i].active == true) {
			if ( cf_get_rand32() % 100 < percent) {
				
				int rv = cf_rcrb_delete(tree, &da->d[i].key);
				if (rv != 0) {
					cf_warning(CF_RB, "fail: could not delete valid value! (%d)",rv);
					return(-1);
				}
				
				da->d[i].active = false;
				n_deleted ++;
			}
		}
	}
	
	if (cf_rcrb_size(tree) + n_deleted != tree_sz) {
		cf_warning(CF_RB, "fail: size after delete is incorrect");
		return(-1);
	}
	
	cf_info(CF_RB, "delete: deleted %d of %d elements",n_deleted,tree_sz);
	
	return(0);	
}

//
// Run a suite of unit tests
//

void cf_rcrb_test_destructor(void *value, void *udata)
{
	cf_rc_free(value);
}


#define RCRB_TEST_SIZE 200000
#define RCRB_TEST_ITERATIONS 50
#define RCRB_TEST_ITERATE_DELETE_PCT 10
#define RCRB_TEST_ITERATE_INSERT_PCT 30

int
cf_rcrb_test()
{
	cf_info(CF_RB, "starting the RCRB test with %d elements\n",RCRB_TEST_SIZE);
	
	// create a new tree
	cf_rcrb_tree *tree = cf_rcrb_create(cf_rcrb_test_destructor , 0);
	
	// create some test data
	test_data_array *da = cf_rcrb_test_make_array( RCRB_TEST_SIZE );
	if (!da) {
		cf_warning(CF_RB, "fail: can't make array");
		cf_rcrb_release(tree, 0);
		return(-1);
	}
	
	// insert all the data
	if (0 != cf_rcrb_test_insert(tree, da, 100)) {
		cf_warning(CF_RB, "fail: can't insert data");
		cf_rcrb_release(tree, 0);
		return(-1);
	}

	// make sure it's there
	if (0 != cf_rcrb_test_validate_data(tree, da)) {
		cf_warning(CF_RB, "fail: can't read data");
		cf_rcrb_release(tree, 0);
		return(-1);
	}

	// validate the tree	
	if (0 != cf_rcrb_validate(tree)) {
		cf_warning(CF_RB, "fail: tree does not validate");
		cf_rcrb_release(tree, 0);
		return -1;
	}
	
	for (int i=0 ; i < RCRB_TEST_ITERATIONS; i++) {
	
		cf_info(CF_RB, "iteraction %d: start: elements in tree %d",i,cf_rcrb_size(tree));
		
		// delete 10 percent of the elements
		if (0 != cf_rcrb_test_delete(tree, da, RCRB_TEST_ITERATE_DELETE_PCT)) {
			cf_warning(CF_RB, "fail: could not delete some lements");
			cf_rcrb_release(tree, 0);
			return -1;
		}
	
		// make sure what's there is or isn't there
		if (0 != cf_rcrb_test_validate_data(tree, da)) {
			cf_warning(CF_RB, "fail: can't read data");
			cf_rcrb_release(tree, 0);
			return(-1);
		}
	
		// validate the tree	
		if (0 != cf_rcrb_validate(tree)) {
			cf_warning(CF_RB, "fail: tree does not validate");
			cf_rcrb_release(tree, 0);
			return -1;
		}

		cf_info(CF_RB, "iteraction %d: after delete: elements in tree %d",i,cf_rcrb_size(tree));
		
		// insert 98 percent of what's not there
		if (0 != cf_rcrb_test_insert(tree, da, RCRB_TEST_ITERATE_INSERT_PCT)) {
			cf_warning(CF_RB, "fail: can't insert data");
			cf_rcrb_release(tree, 0);
			return(-1);
		}
	
		// make sure it's there
		if (0 != cf_rcrb_test_validate_data(tree, da)) {
			cf_warning(CF_RB, "fail: can't read data");
			cf_rcrb_release(tree, 0);
			return(-1);
		}
	
		// validate the tree	
		if (0 != cf_rcrb_validate(tree)) {
			cf_warning(CF_RB, "fail: tree does not validate");
			cf_rcrb_release(tree, 0);
			return -1;
		}
	
	}
	
	// delete the tree
	cf_rcrb_release(tree, 0);
	
	// free the array
	cf_free(da);

	cf_info(CF_RB, "successfully completed the RCRB test with %d elements\n",RCRB_TEST_SIZE);
	
	return(0);
}

