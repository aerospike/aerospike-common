/*
 * A general purpose hashtable implementation
 * Which supports the citrusleaf reference counting
 * natively
 *
 * You can only put a pointer in. Having the pointer in the table holds
 * its reference count. Doing a delete decreases the reference count
 * internally. As 
 * Just, hopefully, the last reasonable hash table you'll ever need
 * Copywrite 2008 Brian Bulkowski
 * All rights reserved
 */

#include <string.h>
#include <stdlib.h>

#include "cf.h"

// this debug tests for reference counts on the object an aweful lot
// #define DEBUG

int
rchash_create(rchash **h_r, rchash_hash_fn h_fn, rchash_destructor_fn d_fn, uint32 key_len, uint32 sz, uint flags)
{
	rchash *h;

	h = malloc(sizeof(rchash));
	if (!h)	return(BB_ERR);

	h->elements = 0;
	h->table_len = sz;
	h->key_len = key_len;
	h->flags = flags;
	h->h_fn = h_fn;
	h->d_fn = d_fn;

	h->table = calloc(sz, sizeof(rchash_elem));
	if (!h->table) {
		free(h);
		return(BB_ERR);
	}

	if (flags & BBHASH_CR_MT_BIGLOCK || flags & BBHASH_CR_MT_LOCKPOOL) {
		if (0 != pthread_mutex_init ( &h->biglock, 0) ) {
			free(h->table); free(h);
			return(BB_ERR);
		}
	}

	*h_r = h;

	return(BB_OK);
}

uint32
rchash_get_size(rchash *h)
{
	return(h->elements);
}

void
rchash_free(rchash *h, void *object)
{
	if (cf_rc_release(object) == 0) {
		
		if (h->d_fn)	(h->d_fn) (object) ;
		cf_rc_free(object);
	}
}

int
rchash_put(rchash *h, void *key, uint32 key_len, void *object)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(BB_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
		
	rchash_elem *e = (rchash_elem *) ( ((uint8_t *)h->table) + (sizeof(rchash_elem) * hash));	

	// most common case should be insert into empty bucket, special case
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		goto Copy;
	}

	rchash_elem *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)	
	while (e) {
#ifdef DEBUG
		if (cf_rc_count(e->object) < 1) {
			D("rchash %p: internal bad reference count on %p",h, e->object);
			return(BB_ERR);
		}
#endif		
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			rchash_free(h,e->object);
			e->object = object;
			if (h->flags & BBHASH_CR_MT_BIGLOCK)
				pthread_mutex_unlock(&h->biglock);
			return(BB_OK);
		}
		e = e->next;
	}

	e = (rchash_elem *) malloc(sizeof(rchash_elem));
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = malloc(key_len);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;

	e->object = object;

	h->elements++;
	if (h->flags & BBHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(BB_OK);	

}

//
// Put of any sort gobbles the reference count.
// make sure the incoming reference count is > 0
//

int
rchash_put_unique(rchash *h, void *key, uint32 key_len, void *object)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(BB_ERR);

	if (cf_rc_count(object) < 1) {
		D("put unique! bad reference count on %p");
		return(BB_ERR);
	}
	
	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
		
	rchash_elem *e = (rchash_elem *) ( ((uint8_t *)h->table) + (sizeof(rchash_elem) * hash));	

	// most common case should be insert into empty bucket, special case
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		goto Copy;
	}

	rchash_elem *e_head = e;

	// check for uniqueness of key - if not unique, fail!
	while (e) {
#ifdef DEBUG
		if (cf_rc_count(e->object) < 1) {
			D("rchash %p: internal bad reference count on %p",h, e->object);
			return(BB_ERR);
		}
#endif		
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			pthread_mutex_unlock(&h->biglock);
			return(BB_ERR_FOUND);
		}
		e = e->next;
	}

	e = (rchash_elem *) malloc(sizeof(rchash_elem));
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = malloc(key_len);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;

	e->object = object;

	h->elements++;
	if (h->flags & BBHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(BB_OK);	

}



int
rchash_get(rchash *h, void *key, uint32 key_len, void **object)
{
	int rv = BB_ERR;
	
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);
	
	rchash_elem *e = (rchash_elem *) ( ((byte *)h->table) + (sizeof(rchash_elem) * hash));	

	do {
#ifdef DEBUG
		if (cf_rc_count(e->object) < 1) {
			D("rchash %p: internal bad reference count on %p",h, e->object);
			return(BB_ERR);
		}
#endif		

		if ( ( key_len == e->key_len ) &&
			 ( memcmp(key, e->key, key_len) == 0) ) {
			cf_rc_reserve( e->object );
			*object = e->object;
			rv = BB_OK; 
			goto Out;
		}
		e = e->next;
	} while (e);
	rv = BB_ERR_NOTFOUND;
	
Out:
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return(rv);
					
}

int
rchash_delete(rchash *h, void *key, uint32 key_len)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(BB_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = BB_ERR;

	if (h->flags & BBHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
		
	rchash_elem *e = (rchash_elem *) ( ((uint8_t *)h->table) + (sizeof(rchash_elem) * hash));	

	// If bucket empty, def can't delete
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		rv = BB_ERR_NOTFOUND;
		goto Out;
	}

	rchash_elem *e_prev = 0;

	// Look for teh element and destroy if found
	while (e) {
		
#ifdef DEBUG
		if (cf_rc_count(e->object) < 1) {
			D("rchash %p: internal bad reference count on %p",h, e->object);
			return(BB_ERR);
		}
#endif		

		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			// Found it, kill it
			free(e->key);
			rchash_free(h, e->object);
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				free (e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					memset(e, 0, sizeof(rchash_elem));
				}
				// at head with a next - more complicated
				else {
					rchash_elem *_t = e->next;
					memcpy(e, e->next, sizeof(rchash_elem));
					free(_t);
				}
			}
			h->elements--;
			rv = BB_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = BB_ERR_NOTFOUND;

Out:
	if (h->flags & RCHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(rv);	
	

}

// Call the function over every node in the tree
// Can be lock-expensive at the moment, until we improve the lockfree code

void
rchash_reduce(rchash *h, rchash_reduce_fn reduce_fn, void *udata)
{
	
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	rchash_elem *he = h->table;
	
	for (uint i=0; i<h->table_len ; i++) {

		rchash_elem *list_he = he;
		while (list_he) {
			
			// 0 length means an unused head pointer - break
			if (list_he->key_len == 0)
				break;
			
#ifdef DEBUG
			if (cf_rc_count(list_he->object) < 1) {
				D("rchash %p: internal bad reference count on %p",h, list_he->object);
			}
#endif		

			reduce_fn(list_he->key, list_he->key_len, list_he->object, udata);
			
			list_he = list_he->next;
		};
		
		he++;
	}

	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return;
}

// A special version of 'reduce' that supports deletion
// In this case, if you return '-1' from the reduce fn, that node will be
// deleted
void
rchash_reduce_delete(rchash *h, rchash_reduce_fn reduce_fn, void *udata)
{
	
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	rchash_elem *he = h->table;
	
	for (uint i=0; i<h->table_len ; i++) {

		rchash_elem *list_he = he;
		rchash_elem *prev_he = 0;
		int rv;

		
		while (list_he) {
			// This kind of structure might have the head as an empty element,
			// that's a signal to move along
			if (list_he->key_len == 0)
				break;
			
#ifdef DEBUG
			if (cf_rc_count(list_he->object) < 1) {
				D("rchash %p: internal bad reference count on %p",h, list_he->object);
				return(BB_ERR);
			}
#endif		
			
			rv = reduce_fn(list_he->key, list_he->key_len, list_he->object, udata);
			
			// Delete is requested
			// Leave the pointers in a "next" state
			if (rv == -1) {
				free(list_he->key);
				rchash_free(h, list_he->object);
				// patchup pointers & free element if not head
				if (prev_he) {
					prev_he->next = list_he->next;
					free (list_he);
					list_he = prev_he->next;
				}
				// am at head - more complicated
				else {
					// at head with no next - easy peasy!
					if (0 == list_he->next) {
						memset(list_he, 0, sizeof(rchash_elem));
						list_he = 0;
					}
					// at head with a next - more complicated -
					// copy next into current and free next
					// (the old trick of how to delete from a singly
					// linked list without a prev pointer)
					// Somewhat confusingly, prev_he stays 0
					// and list_he stays where it is
					else {
						rchash_elem *_t = list_he->next;
						memcpy(list_he, list_he->next, sizeof(rchash_elem));
						free(_t);
					}
				}

			}
			else { // don't delete, just forward everything
				prev_he = list_he;
				list_he = list_he->next;
			}	
				
		};
		
		he++;
	}

	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return;
}


void
rchash_destroy(rchash *h)
{
	rchash_elem *e_table = h->table;
	for (uint i=0;i<h->table_len;i++) {
		if (e_table->next) {
			rchash_elem *e = e_table->next;
			rchash_elem *t;
			while (e) {
				t = e->next;
				free(e);
				e = t;
			}
		}
		e_table = (rchash_elem *) ( ((byte *) e_table) + sizeof(rchash_elem) );
	}

	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_destroy(&h->biglock);


	free(h->table);
	free(h);	
}	
