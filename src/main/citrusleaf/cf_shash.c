/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

/**
 * A general purpose hashtable implementation
 * Good at multithreading
 * Just, hopefully, the last reasonable hash table you'll ever need
 */

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <citrusleaf/cf_shash.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * shash_create
 * Create a simple hash table.
 * (Note:  For supporting memory allocation counting / tracking, we need to be able to create a single
 *          hash table that is untracked, in order to avoid circularity.  In this special case only,
 *          the native (i.e., non-"cf_") versions of memory allocation / freeing functions *must* be used.)
 * (Also note:  Memory for the elements in a tracked hash table is tracked, along with memory for
 *               the hash table itself.) 
 */
int shash_create(shash **h_r, shash_hash_fn h_fn, uint32_t key_len, uint32_t value_len, uint32_t sz, uint flags) {
	shash *h;
	bool mem_tracked = !(flags & SHASH_CR_UNTRACKED);

	h = (shash *) (mem_tracked ? cf_malloc(sizeof(shash)) : malloc(sizeof(shash)));
	if (!h)	return(SHASH_ERR);

	h->elements = 0;
	h->table_len = sz;
	h->key_len = key_len;
	h->value_len = value_len;
	h->flags = flags;
	h->h_fn = h_fn;

	if ((flags & SHASH_CR_MT_BIGLOCK) && (flags & SHASH_CR_MT_MANYLOCK)) {
		*h_r = 0;
		return(SHASH_ERR);
	}
	
	h->table = (shash_elem *) (mem_tracked ? cf_malloc(sz * SHASH_ELEM_SZ(h)) : malloc(sz * SHASH_ELEM_SZ(h)));

	if (!h->table) {
		if (mem_tracked) 
		  cf_free(h);
		else
		  free(h);
		*h_r = 0;
		return(SHASH_ERR);
	}
	
	shash_elem *table = h->table;
	for (uint i=0;i<sz;i++) {
		table->in_use = false;
		table->next = 0;
		// next element in head table
		table = (shash_elem *) (((uint8_t *)table) + SHASH_ELEM_SZ(h));
	}
	
	if (flags & SHASH_CR_MT_BIGLOCK) {
		if (0 != pthread_mutex_init ( &h->biglock, 0) ) {
			if (mem_tracked) {
				cf_free(h->table);
				cf_free(h);
			} else {
				free(h->table);
				free(h);
			}
			return(SHASH_ERR);
		}
	}
	else
		memset( (void *) &h->biglock, 0, sizeof( h->biglock ) );
	
	if (flags & SHASH_CR_MT_MANYLOCK) {
		h->lock_table = (pthread_mutex_t *) (mem_tracked ? cf_malloc( sizeof(pthread_mutex_t) * sz) : malloc( sizeof(pthread_mutex_t) * sz));
		if (! h->lock_table) {
			if (mem_tracked)
			  cf_free(h);
			else
			  free(h);
			*h_r = 0;
			return(SHASH_ERR);
		}
		for (uint i=0;i<sz;i++) {
			pthread_mutex_init( &(h->lock_table[i]), 0 );
		}
	}
	else
		h->lock_table = 0;

	*h_r = h;

	return(SHASH_OK);
}

/**
 * If MANYLOCK, then the elements counter will be wrong because there's no single
 * lock to use to protect. We've got a choice to use an atomic to make sure
 * the get_size call is fast in that case (thus slowing down every access), or
 * running the list and make get_size slow.
 * Choose making get_size slow in the case, but retaining parallelism
 */
uint32_t shash_get_size(shash *h) {
    uint32_t elements = 0;
	
	if (h->flags & SHASH_CR_MT_MANYLOCK) {
        
        for (uint i=0; i<h->table_len ; i++) {
            
            pthread_mutex_t *l = &(h->lock_table[i]);
            pthread_mutex_lock( l );
            
            shash_elem *list_he = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * i));
            while (list_he) {
                
                // not in use is common at head pointer - unused bucket
                if (list_he->in_use == false) 
                    break;
                
                elements++;
                
                list_he = list_he->next;
            }
    
            pthread_mutex_unlock(l);
    
        }

    }
    
    else if (h->flags & SHASH_CR_MT_BIGLOCK) {
    	pthread_mutex_lock(&h->biglock);
    	elements = h->elements;
    	pthread_mutex_unlock(&h->biglock);
    }
    else {
    	elements = h->elements;
    }
    
	return(elements);
}

int shash_put(shash *h, void *key, void *value) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	// Calculate hash
	uint hash = h->h_fn(key);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	shash_elem *e = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * hash));	

	// most common case should be insert into empty bucket, special case
	if ( e->in_use == false )
		goto Copy;

	shash_elem *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)	
	while (e) {
		if (memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			memcpy(SHASH_ELEM_VALUE_PTR(h, e), value, h->value_len);
			if (l)     pthread_mutex_unlock(l);
			return(SHASH_OK);
		}
		e = e->next;
	}

	e = (shash_elem *) (mem_tracked ? cf_malloc(SHASH_ELEM_SZ(h)) : malloc(SHASH_ELEM_SZ(h)));
	if (!e) {
		if (l)     pthread_mutex_unlock( l );
		return (SHASH_ERR);
	}

	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len);
	memcpy(SHASH_ELEM_VALUE_PTR(h, e), value, h->value_len);
	e->in_use = true;
	h->elements++;
	if (l)	pthread_mutex_unlock( l );
	return(SHASH_OK);	
}

// Fail if there's already a value there

int shash_put_unique(shash *h, void *key, void *value) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	// Calculate hash
	uint hash = h->h_fn(key);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	shash_elem *e = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * hash));	

	// most common case should be insert into empty bucket, special case
	if ( e->in_use == false ) {
		goto Copy;
	}

	shash_elem *e_head = e;

	while (e) {
		if (memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			if (l)     pthread_mutex_unlock(l);
			return(SHASH_ERR_FOUND);
		}
		e = e->next;
	}

	e = (shash_elem *) (mem_tracked ? cf_malloc(SHASH_ELEM_SZ(h)) : malloc(SHASH_ELEM_SZ(h)));
	if (!e) {
		if (l)     pthread_mutex_unlock( l );
		return (SHASH_ERR);
	}

	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len);
	memcpy(SHASH_ELEM_VALUE_PTR(h, e), value, h->value_len);
	e->in_use = true;
	h->elements++;
	if (l)	pthread_mutex_unlock( l );
	return(SHASH_OK);	

}

/**
 * Put duplicate elements, Use case currently is hash/search/delete
 * i.e Once the element is found it is deleted. Only changes is the 
 * presence of the key is not searched for.
 */
int shash_put_duplicate(shash *h, void *key, void *value) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	// Calculate hash
	uint hash = h->h_fn(key);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	shash_elem *e = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * hash));	
	shash_elem *e_head = e;
	// most common case should be insert into empty bucket, special case
	if ( e->in_use == false )
		goto Copy;

	e = (shash_elem *) (mem_tracked ? cf_malloc(SHASH_ELEM_SZ(h)) : malloc(SHASH_ELEM_SZ(h)));
	if (!e) {
		if (l)     pthread_mutex_unlock( l );
		return (SHASH_ERR);
	}

	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len);
	memcpy(SHASH_ELEM_VALUE_PTR(h, e), value, h->value_len);
	e->in_use = true;
	h->elements++;
	if (l)	pthread_mutex_unlock( l );
	return(SHASH_OK);	
}

int shash_get(shash *h, void *key, void *value) {
	int rv = SHASH_ERR;

	uint hash = h->h_fn(key);
	hash %= h->table_len;

	pthread_mutex_t *l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );

	shash_elem *e = (shash_elem *) (((byte *)h->table) + (SHASH_ELEM_SZ(h) * hash));

	if (e->in_use == false) {
		rv = SHASH_ERR_NOTFOUND;
		goto Out;
	}

	do {
		if (memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			if (NULL != value)
			    memcpy(value, SHASH_ELEM_VALUE_PTR(h, e), h->value_len);
			rv = SHASH_OK; 
			goto Out;
		}
		e = e->next;
	} while (e);
	rv = SHASH_ERR_NOTFOUND;

Out:
	if (l) pthread_mutex_unlock(l);

	return(rv);
}

/**
 * Note that the vlock is passed back only when the return code is SHASH_OK.
 * In the case where nothing is found, no lock is held.
 * It might be better to do it the other way, but you can change it later if you want
 */
int shash_get_vlock(shash *h, void *key, void **value, pthread_mutex_t **vlock) {
	int rv = SHASH_ERR;
	
	uint hash = h->h_fn(key);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
	
	shash_elem *e = (shash_elem *) ( ((byte *)h->table) + (SHASH_ELEM_SZ(h) * hash));	

	if (e->in_use == false) {
		rv = SHASH_ERR_NOTFOUND;
		goto Out;
	}
	
	do {
		if ( memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			*value = SHASH_ELEM_VALUE_PTR(h, e);
			rv = SHASH_OK; 
			goto Out;
		}
		e = e->next;
	} while (e);
	
	rv = SHASH_ERR_NOTFOUND;
	
Out:
	if (l) {
		if (rv == SHASH_OK) {
			*vlock = l;
		}
		else {
			pthread_mutex_unlock( l );
			*vlock = 0;
		}
	}
	else {
		// for safety sake
		if (vlock) *vlock = 0;
	}

	return(rv);
					
}

/**
 * Atomically update an entry in the hash table using a user-supplied update function and user data.
 * The old and new values must be allocated by the caller.
 * The user data can be anything.
 */
int shash_update(shash *h, void *key, void *value_old, void *value_new, shash_update_fn update_fn, void *udata) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	uint hash = h->h_fn(key);
	hash %= h->table_len;
	int rv = SHASH_OK;

	pthread_mutex_t *l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );

	shash_elem *e = (shash_elem *) (((byte *)h->table) + (SHASH_ELEM_SZ(h) * hash));
	shash_elem *e_head = e;

	if (e->in_use == false) {
		value_old = NULL;
		goto Update;
	}

	do {
		if (memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			if (NULL != value_old)
			    memcpy(value_old, SHASH_ELEM_VALUE_PTR(h, e), h->value_len);
			goto Update;
		}
		e = e->next;
	} while (e);
	value_old = NULL;

Update:
	// Invoke the caller's update function.
	(update_fn)(key, value_old, value_new, udata);

	// Write the new value into the hash table.

	if (!value_old && !e) {
		e = (shash_elem *) (mem_tracked ? cf_malloc(SHASH_ELEM_SZ(h)) : malloc(SHASH_ELEM_SZ(h)));
		if (!e) {
			if (l)     pthread_mutex_unlock( l );
			return (SHASH_ERR);
		}

		e->next = e_head->next;
		e_head->next = e;
	}

	if (!value_old)
	  memcpy(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len);
	memcpy(SHASH_ELEM_VALUE_PTR(h, e), value_new, h->value_len);
	e->in_use = true;

	if (!value_old)
	  h->elements++;

	if (l) pthread_mutex_unlock(l);

	return(rv);
}

int shash_delete(shash *h, void *key) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	// Calculate hash
	uint hash = h->h_fn(key);
	hash %= h->table_len;
	int rv = SHASH_ERR;

	pthread_mutex_t		*l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	shash_elem *e = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * hash));	

	// If bucket empty, def can't delete
	if ( e->in_use == false ) {
		rv = SHASH_ERR_NOTFOUND;
		goto Out;
	}

	shash_elem *e_prev = 0;

	// Look for the element and destroy if found
	while (e) {
		if ( memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			// Found it
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				if (mem_tracked)
				  cf_free(e);
				else
				  free(e);				
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					e->in_use = false;
				}
				// at head with a next - more complicated
				else {
					shash_elem *_t = e->next;
					memcpy(e, e->next, SHASH_ELEM_SZ(h) );
					if (mem_tracked)
					  cf_free(_t);
					else
					  free(_t);
				}
			}
			h->elements--;
			rv = SHASH_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = SHASH_ERR_NOTFOUND;

Out:
	if (l) 
		pthread_mutex_unlock(l);
	return(rv);	
	

}

// I kind of feel bad about this cut n paste, but
// it's nice in the lock state to slim down the area under which the lock is taken
// doing multiple helper functions may be better...

int shash_delete_lockfree(shash *h, void *key) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	// Calculate hash
	uint hash = h->h_fn(key);
	hash %= h->table_len;

	shash_elem *e = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * hash));	

	// If bucket empty, def can't delete
	if ( e->in_use == false )
		return( SHASH_ERR_NOTFOUND );

	shash_elem *e_prev = 0;

	// Look for the element and destroy if found
	while (e) {
		if ( memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
			// Found it
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				if (mem_tracked)
				  cf_free(e);
				else
				  free(e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					e->in_use = false;
				}
				// at head with a next - more complicated
				else {
					shash_elem *_t = e->next;
					memcpy(e, e->next, SHASH_ELEM_SZ(h) );
					if (mem_tracked)
					  cf_free(_t);
					else
					  free(_t);
				}
			}
			h->elements--;
			return( SHASH_OK );
		}
		e_prev = e;
		e = e->next;
	}
	
	return ( SHASH_ERR_NOTFOUND );

	
}

int shash_get_and_delete(shash *h, void *key, void *value) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	// Calculate hash
	uint hash = h->h_fn(key);
	hash %= h->table_len;
	int rv = SHASH_ERR;

	pthread_mutex_t		*l = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & SHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	shash_elem *e = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * hash));	

	// If bucket empty, def can't delete
	if ( e->in_use == false ) {
		rv = SHASH_ERR_NOTFOUND;
		goto Out;
	}

	shash_elem *e_prev = 0;

	// Look for the element and destroy if found
	while (e) {
		if ( memcmp(SHASH_ELEM_KEY_PTR(h, e), key, h->key_len) == 0) {
		
			// Found it - check length of buffer and copy to destination
			memcpy(value, SHASH_ELEM_VALUE_PTR(h, e), h->value_len);

			
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				if (mem_tracked)
				  cf_free(e);
				else
				  free(e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					e->in_use = false;
				}
				// at head with a next - more complicated
				else {
					shash_elem *_t = e->next;
					memcpy(e, e->next, SHASH_ELEM_SZ(h) );
					if (mem_tracked)
					  cf_free(_t);
					else
					  free(_t);
				}
			}
			h->elements--;
			rv = SHASH_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = SHASH_ERR_NOTFOUND;

Out:
	if (l)
		pthread_mutex_unlock( l );
		
	return(rv);	
	

}

/**
 * Call the function over every node in the tree
 * Can be lock-expensive at the moment, until we improve the lockfree code
 * the return value is the non-zero return value of any of the reduce calls,
 * if there was one that didn't return zero.
 */
int shash_reduce(shash *h, shash_reduce_fn reduce_fn, void *udata) {
	int rv = 0;
	
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock( &h->biglock);
	}

	for (uint i=0; i<h->table_len ; i++) {
		
		pthread_mutex_t *l = 0;
		if (h->flags & SHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}
		
		shash_elem *list_he = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * i));
		while (list_he) {
			
			// not in use is common at head pointer - unused bucket
			if (list_he->in_use == false)
				break;
			
			rv = reduce_fn(	SHASH_ELEM_KEY_PTR(h, list_he), SHASH_ELEM_VALUE_PTR(h, list_he), udata);
			if (0 != rv) {
                if (l) pthread_mutex_unlock(l);
				goto Out;
			}
			
			list_he = list_he->next;
		}

		if (l)	pthread_mutex_unlock(l);

	}
Out:
	if (h->flags & SHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return(rv);
}

/**
 * A special version of 'reduce' that supports deletion
 * In this case, if you return '1' from the reduce fn, that node will be
 * deleted. All other return values will terminate the reduce and pass
 * that value back to the caller. We're using '1' because in this codebase,
 * negative numbers are errors
 */
int shash_reduce_delete(shash *h, shash_reduce_fn reduce_fn, void *udata) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);
	int rv = 0;
	
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock( &h->biglock);
	}
	
	for (uint i=0; i<h->table_len ; i++) {

		pthread_mutex_t *l = 0;
		if (h->flags & SHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}
		
		shash_elem *list_he = (shash_elem *) ( ((uint8_t *)h->table) + (SHASH_ELEM_SZ(h) * i));
		shash_elem *prev_he = 0;
		
		while (list_he) {
			// This kind of structure might have the head as an empty element,
			// that's a signal to move along
			if (list_he->in_use == false)
				break;
			
			rv = reduce_fn( SHASH_ELEM_KEY_PTR(h, list_he), SHASH_ELEM_VALUE_PTR(h, list_he), udata);
			
			// Delete is requested
			// Leave the pointers in a "next" state
			if (rv == SHASH_REDUCE_DELETE) {

                h->elements--;
                
				// patchup pointers & free element if not head
				if (prev_he) {
					prev_he->next = list_he->next;
					if (mem_tracked)
					  cf_free(list_he);
					else
					  free(list_he);					
					list_he = prev_he->next;
				}
				// am at head - more complicated
				else {
					// at head with no next - easy peasy!
					if (0 == list_he->next) {
						list_he->in_use = false;
						list_he = 0;
					}
					// at head with a next - more complicated -
					// copy next into current and free next
					// (the old trick of how to delete from a singly
					// linked list without a prev pointer)
					// Somewhat confusingly, prev_he stays 0
					// and list_he stays where it is
					else {
						shash_elem *_t = list_he->next;
						memcpy(list_he, list_he->next, SHASH_ELEM_SZ(h) );
						if (mem_tracked)
						  cf_free(_t);
						else
						  free(_t);
					}
				}
				rv = 0;
			}
			else if (0 != rv) {
				if (l)	pthread_mutex_unlock(l);
				goto Out;
			}
			else { // don't delete, just forward everything
				prev_he = list_he;
				list_he = list_he->next;
			}	
		}
		
		if (l) pthread_mutex_unlock(l);
	}

Out:

	if (h->flags & SHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock( &h->biglock);
	
	return(rv);
}

/**
 * Remove all the hashed keys from the hash bucket if the caller 
 * knows this is going to be single threaded
 */
void shash_deleteall_lockfree(shash *h) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	shash_elem *e_table = h->table;
	for (uint i=0;i<h->table_len;i++) {
		if (e_table->next) {
			shash_elem *e = e_table->next;
			shash_elem *t;
			while (e) {
				t = e->next;
				if (mem_tracked)
				  cf_free(e);
				else
				  free(e);
				e = t;
			}
			// The head element of each hash bucket overflow chain also
			// contains data. But we should not free it as it is 
			// allocated as part of the overall hash table. So, just mark
			// it so that it is re-used.
			e_table->next = NULL;
		}
		e_table->in_use = false;
		e_table = (shash_elem *) (((uint8_t *)e_table) + SHASH_ELEM_SZ(h));
	}
	h->elements = 0;
}

void shash_deleteall(shash *h) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);
	
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}
	
	shash_elem *e_table = h->table;
	for (uint i=0;i<h->table_len;i++) {
		pthread_mutex_t *l = 0;
		if (h->flags & SHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}
		
		if (e_table->next) {
			shash_elem *e = e_table->next;
			shash_elem *t;
			while (e) {
				t = e->next;
				if (mem_tracked)
				  cf_free(e);
				else
				  free(e);
				e = t;
			}
			// The head element of each hash bucket overflow chain also
			// contains data. But we should not free it as it is·
			// allocated as part of the overall hash table. So, just mark
			// it so that it is re-used.
			e_table->next = NULL;
		}
		e_table->in_use = false;
		if (l) {
			pthread_mutex_unlock(l);
		}
		e_table = (shash_elem *) (((uint8_t *)e_table) + SHASH_ELEM_SZ(h));
	}
	h->elements = 0;
	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		pthread_mutex_unlock(&h->biglock);
	}
}

/**
 * shash_destroy
 * Destroy a simple hash table. 
 */
void shash_destroy(shash *h) {
	bool mem_tracked = !(h->flags & SHASH_CR_UNTRACKED);

	shash_elem * e_table = (shash_elem *) h->table;
	for (uint i=0;i<h->table_len;i++) {
		if (e_table->next) {
			shash_elem *e = e_table->next;
			shash_elem *t;
			while (e) {
				t = e->next;
				if (mem_tracked)
				  cf_free(e);
				else
				  free(e);
				e = t;
			}
		}
		e_table = (shash_elem *) (((uint8_t *)e_table) + SHASH_ELEM_SZ(h));
	}

	if (h->flags & SHASH_CR_MT_BIGLOCK) {
		pthread_mutex_destroy(&h->biglock);
	}
	if (h->flags & SHASH_CR_MT_MANYLOCK) {
		for (uint i=0;i<h->table_len;i++) {
			pthread_mutex_destroy(&(h->lock_table[i]));
		}
		if (mem_tracked)
		  cf_free(h->lock_table);
		else
		  free(h->lock_table);
	}

	if (mem_tracked) {
		cf_free(h->table);
		cf_free(h);
	} else {
		free(h->table);
		free(h);
	}
}	
