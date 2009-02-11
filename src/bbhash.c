/*
 * A general purpose hashtable implementation
 * Good at multithreading
 * Just, hopefully, the last reasonable hash table you'll ever need
 * Copywrite 2008 Brian Bulkowski
 * All rights reserved
 */

#include <string.h>
#include <stdlib.h>

#include "cf.h"
// standalone version is different
// #include "bbhash.h"

int
bbhash_create(bbhash **h_r, bbhash_hash_fn h_fn, uint32 key_len, uint32 value_len, uint32 sz, uint flags)
{
	bbhash *h;

	h = malloc(sizeof(bbhash));
	if (!h)	return(BB_ERR);

	h->elements = 0;
	h->table_len = sz;
	h->key_len = key_len;
	h->value_len = value_len;
	h->flags = flags;
	h->h_fn = h_fn;

	h->table = calloc(sz, sizeof(bbhash_elem));
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
bbhash_get_size(bbhash *h)
{
	return(h->elements);
}

int
bbhash_put(bbhash *h, void *key, uint32 key_len, void *value, uint32 value_len)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(BB_ERR);
	if ((h->value_len) && (h->value_len != value_len) ) return(BB_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	if (h->flags & BBHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
		
	bbhash_elem *e = (bbhash_elem *) ( ((uint8_t *)h->table) + (sizeof(bbhash_elem) * hash));	

	// most common case should be insert into empty bucket, special case
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		goto Copy;
	}

	bbhash_elem *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)	
	while (e) {
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			free(e->value);
			e->value = malloc(value_len);
			memcpy(e->value, value, value_len);
			if (h->flags & BBHASH_CR_MT_BIGLOCK)
				pthread_mutex_unlock(&h->biglock);
			return(BB_OK);
		}
		e = e->next;
	}

	e = (bbhash_elem *) malloc(sizeof(bbhash_elem));
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = malloc(key_len);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;
	e->value = malloc(value_len);
	memcpy(e->value, value, value_len);
	e->value_len = value_len;
	h->elements++;
	if (h->flags & BBHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(BB_OK);	

}

int
bbhash_get(bbhash *h, void *key, uint32 key_len, void *value, uint32 *value_len)
{
	int rv = BB_ERR;
	
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);
	
	bbhash_elem *e = (bbhash_elem *) ( ((byte *)h->table) + (sizeof(bbhash_elem) * hash));	

	do {
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(key, e->key, key_len) == 0) ) {
			if ( *value_len < e->value_len ) {
				*value_len = e->value_len; // give em a hint so they can call again
				rv = BB_ERR_BUFSZ; 
				goto Out;
			}
			memcpy(value, e->value, e->value_len);
			*value_len = e->value_len;
			rv = BB_OK; 
			goto Out;
		}
		e = e->next;
	} while (e);
	rv = BB_ERR_NOTFOUND;
	
Out:
	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return(rv);
					
}

int
bbhash_delete(bbhash *h, void *key, uint32 key_len)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(BB_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = BB_ERR;

	if (h->flags & BBHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
		
	bbhash_elem *e = (bbhash_elem *) ( ((uint8_t *)h->table) + (sizeof(bbhash_elem) * hash));	

	// If bucket empty, def can't delete
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		rv = BB_ERR_NOTFOUND;
		goto Out;
	}

	bbhash_elem *e_prev = 0;

	// Look for teh element and destroy if found
	while (e) {
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			// Found it
			free(e->key);
			free(e->value);
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				free (e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					memset(e, 0, sizeof(bbhash_elem));
				}
				// at head with a next - more complicated
				else {
					bbhash_elem *_t = e->next;
					memcpy(e, e->next, sizeof(bbhash_elem));
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
	if (h->flags & BBHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(rv);	
	

}

int
bbhash_get_and_delete(bbhash *h, void *key, uint32 key_len, void *value, uint32 *value_len)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(BB_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = BB_ERR;

	if (h->flags & BBHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
		
	bbhash_elem *e = (bbhash_elem *) ( ((uint8_t *)h->table) + (sizeof(bbhash_elem) * hash));	

	// If bucket empty, def can't delete
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		rv = BB_ERR_NOTFOUND;
		goto Out;
	}

	bbhash_elem *e_prev = 0;

	// Look for teh element and destroy if found
	while (e) {
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
		
			// Found it - check length of buffer and copy to destination
			if (*value_len < e->value_len) {
				*value_len = e->value_len;
				rv = -2;
				goto Out;
			}
			memcpy(value, e->value, e->value_len);
			*value_len = e->value_len;
			
			// free the underlying data
			free(e->key);
			free(e->value);
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				free (e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					memset(e, 0, sizeof(bbhash_elem));
				}
				// at head with a next - more complicated
				else {
					bbhash_elem *_t = e->next;
					memcpy(e, e->next, sizeof(bbhash_elem));
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
	if (h->flags & BBHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(rv);	
	

}



// Call the function over every node in the tree
// Can be lock-expensive at the moment, until we improve the lockfree code
// the return value is the non-zero return value of any of the reduce calls,
// if there was one that didn't return zero.
int
bbhash_reduce(bbhash *h, bbhash_reduce_fn reduce_fn, void *udata)
{
	int rv = 0;
	
	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	bbhash_elem *he = h->table;
	
	for (uint i=0; i<h->table_len ; i++) {

		bbhash_elem *list_he = he;
		while (list_he) {
			
			// 0 length means an unused head pointer - break
			if (list_he->key_len == 0)
				break;
			
			rv = reduce_fn(list_he->key, list_he->key_len, list_he->value, list_he->value_len, udata);
			if (0 != rv)
				goto Out;
			
			list_he = list_he->next;
		};
		
		he++;
	}
Out:
	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return(rv);
}

// A special version of 'reduce' that supports deletion
// In this case, if you return '1' from the reduce fn, that node will be
// deleted. All other return values will terminate the reduce and pass
// that value back to the caller. We're using '1' because in this codebase,
// negative numbers are errors
int
bbhash_reduce_delete(bbhash *h, bbhash_reduce_fn reduce_fn, void *udata)
{
	int rv = 0;
	
	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	bbhash_elem *he = h->table;
	
	for (uint i=0; i<h->table_len ; i++) {

		bbhash_elem *list_he = he;
		bbhash_elem *prev_he = 0;
		
		while (list_he) {
			// This kind of structure might have the head as an empty element,
			// that's a signal to move along
			if (list_he->key_len == 0)
				break;
			
			rv = reduce_fn(list_he->key, list_he->key_len, list_he->value, list_he->value_len, udata);
			
			// Delete is requested
			// Leave the pointers in a "next" state
			if (rv == BBHASH_REDUCE_DELETE) {
				free(list_he->key);
				free(list_he->value);
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
						memset(list_he, 0, sizeof(bbhash_elem));
						list_he = 0;
					}
					// at head with a next - more complicated -
					// copy next into current and free next
					// (the old trick of how to delete from a singly
					// linked list without a prev pointer)
					// Somewhat confusingly, prev_he stays 0
					// and list_he stays where it is
					else {
						bbhash_elem *_t = list_he->next;
						memcpy(list_he, list_he->next, sizeof(bbhash_elem));
						free(_t);
					}
				}
				rv = 0;
			}
			else if (0 != rv) {
				goto Out;
			}
			else { // don't delete, just forward everything
				prev_he = list_he;
				list_he = list_he->next;
			}	
		};
		
		he++;
	}

Out:
	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return(rv);
}


void
bbhash_destroy(bbhash *h)
{
	bbhash_elem *e_table = h->table;
	for (uint i=0;i<h->table_len;i++) {
		if (e_table->next) {
			bbhash_elem *e = e_table->next;
			bbhash_elem *t;
			while (e) {
				t = e->next;
				free(e->key);
				free(e->value);
				free(e);
				e = t;
			}
		}
		e_table = (bbhash_elem *) ( ((byte *) e_table) + sizeof(bbhash_elem) );
	}

	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_destroy(&h->biglock);


	free(h->table);
	free(h);	
}	
