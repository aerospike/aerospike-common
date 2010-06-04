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
// #define VALIDATE

void rchash_destroy_v(rchash *h);
void rchash_reduce_delete_v(rchash *h, rchash_reduce_fn reduce_fn, void *udata);
void rchash_reduce_v(rchash *h, rchash_reduce_fn reduce_fn, void *udata);
int rchash_delete_v(rchash *h, void *key, uint32_t key_len);
int rchash_get_v(rchash *h, void *key, uint32_t key_len, void **object);
int rchash_put_unique_v(rchash *h, void *key, uint32_t key_len, void *object);
int rchash_put_v(rchash *h, void *key, uint32_t key_len, void *object);
uint32_t rchash_get_size_v(rchash *h);
void rchash_destroy_elements_v(rchash *h);



int
rchash_create(rchash **h_r, rchash_hash_fn h_fn, rchash_destructor_fn d_fn, uint32_t key_len, uint32_t sz, uint flags)
{
	rchash *h;

	h = CF_MALLOC(sizeof(rchash));
	if (!h)	return(RCHASH_ERR);

	h->elements = 0;
	h->table_len = sz;
	h->key_len = key_len;
	h->flags = flags;
	h->h_fn = h_fn;
	h->d_fn = d_fn;

	if ((flags & RCHASH_CR_MT_BIGLOCK) && (flags & RCHASH_CR_MT_MANYLOCK)) {
		*h_r = 0;
		return(RCHASH_ERR);
	}

	if (key_len == 0)
        h->table = calloc(sz, sizeof(rchash_elem_v));
    else
        h->table = calloc(sz, sizeof(rchash_elem_f) + key_len);
    
	if (!h->table) {
		free(h);
		return(RCHASH_ERR);
	}

	if (flags & RCHASH_CR_MT_BIGLOCK) {
		if (0 != pthread_mutex_init ( &h->biglock, 0) ) {
			free(h->table); free(h);
			return(SHASH_ERR);
		}
	}
	else
		memset( &h->biglock, 0, sizeof( h->biglock ) );
	
	if (flags & RCHASH_CR_MT_MANYLOCK) {
		h->lock_table = CF_MALLOC( sizeof(pthread_mutex_t) * sz);
		if (! h->lock_table) {
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

	return(RCHASH_OK);
}

void
rchash_free(rchash *h, void *object)
{
	if (cf_rc_release(object) == 0) {
		if (h->d_fn)	(h->d_fn) (object) ;
		cf_rc_free(object);
	}
}

static inline
rchash_elem_f *get_bucket(rchash *h, uint i)
{
    return( (rchash_elem_f * ) (
                ((uint8_t *) h->table) +
                ((sizeof(rchash_elem_f) + h->key_len) * i) 
             )
           );
}

uint32_t
rchash_get_size(rchash *h)
{
    if (h->key_len == 0)    return(rchash_get_size_v(h));
    
    uint32_t sz = 0;
    
    if (h->flags & RCHASH_CR_MT_BIGLOCK) {
    	pthread_mutex_lock(&h->biglock);
    	sz = h->elements;
    	pthread_mutex_unlock(&h->biglock);
    }
    else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
    	sz = cf_atomic32_get(h->elements);
    }
    else {
    	sz = h->elements;
    }

// interesting working code to spin through a table, taking locks, to find the exact size
// written for manylock case only
#if 0    
	uint32_t validate_size = 0;

	for (uint i=0; i<h->table_len ; i++) {

		pthread_mutex_t *l = &(h->lock_table[i]);
		pthread_mutex_lock( l );

		rchash_elem_f *list_he = get_bucket(h, i);	

		while (list_he) {
			// null object means unused head pointer
			if (list_he->object == 0)
				break;
			validate_size++;
			list_he = list_he->next;
		};
		
		pthread_mutex_unlock(l);
		
	}
#endif

    return(sz);
    
}

int
rchash_put(rchash *h, void *key, uint32_t key_len, void *object)
{
    if (h->key_len == 0)    return(rchash_get_size_v(h));

	if (h->key_len != key_len) return(RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	rchash_elem_f *e = get_bucket(h, hash);	

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0  ) {
		goto Copy;
	}

	rchash_elem_f *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)	
	while (e) {
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)		pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		
        // in this case we're replacing the previous object with the new object
		if ( memcmp(e->key, key, key_len) == 0) {
			rchash_free(h,e->object);
			e->object = object;
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_OK);
		}
		e = e->next;
	}

	e = (rchash_elem_f *) CF_MALLOC(sizeof(rchash_elem_f) + key_len);
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(e->key, key, key_len);

	e->object = object;

	if (h->flags & RCHASH_CR_MT_MANYLOCK)
		cf_atomic32_incr(&h->elements);
	else
		h->elements++;
	
	if (l)		pthread_mutex_unlock(l);
	return(RCHASH_OK);	

}

//
// Put of any sort gobbles the reference count.
// make sure the incoming reference count is > 0
//

int
rchash_put_unique(rchash *h, void *key, uint32_t key_len, void *object)
{
    if (h->key_len == 0)    return(rchash_put_unique_v(h,key,key_len,object));

	if (h->key_len != key_len) return(RCHASH_ERR);

#ifdef VALIDATE
	if (cf_rc_count(object) < 1) {
		cf_info(CF_RCHASH,"put unique! bad reference count on %p");
		return(RCHASH_ERR);
	}
#endif    
	
	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	rchash_elem_f *e = get_bucket(h, hash);	

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0 ) goto Copy;

	rchash_elem_f *e_head = e;

	// check for uniqueness of key - if not unique, fail!
	while (e) {
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		
		if ( memcmp(e->key, key, key_len) == 0) {
			if (l) pthread_mutex_unlock(l);
			return(RCHASH_ERR_FOUND);
		}
		e = e->next;
	}

	e = (rchash_elem_f *) CF_MALLOC(sizeof(rchash_elem_f) + key_len);
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(e->key, key, key_len);

	e->object = object;

	if (h->flags & RCHASH_CR_MT_MANYLOCK)
		cf_atomic32_incr(&h->elements);
	else
		h->elements++;

	if (l)		pthread_mutex_unlock(l);
	return(RCHASH_OK);	

}



int
rchash_get(rchash *h, void *key, uint32_t key_len, void **object)
{
    if (h->key_len == 0)    return(rchash_get_v(h,key,key_len,object));
	if (h->key_len != key_len) return(RCHASH_ERR);

	int rv = RCHASH_ERR;
	
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
	
	rchash_elem_f *e = get_bucket(h, hash);	

  	// most common case should be insert into empty bucket, special case
	if ( e->object == 0 ) {
        rv = RCHASH_ERR_NOTFOUND;
        goto Out;
	}
    
	while (e) {
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		

		if ( memcmp(key, e->key, key_len) == 0) {
			cf_rc_reserve( e->object );
			*object = e->object;
			rv = RCHASH_OK; 
			goto Out;
		}
		e = e->next;
	};
    
	rv = RCHASH_ERR_NOTFOUND;
	
Out:
	if (l)
		pthread_mutex_unlock(l);

	return(rv);
					
}

int
rchash_delete(rchash *h, void *key, uint32_t key_len)
{
    if (h->key_len == 0)    return(rchash_delete_v(h,key,key_len));
	if (h->key_len != key_len) return(RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = RCHASH_ERR;

    // take lock
	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	rchash_elem_f *e = get_bucket(h, hash);	

	// If bucket empty, def can't delete
	if ( e->object == 0 ) {
		rv = RCHASH_ERR_NOTFOUND;
		goto Out;
	}

	rchash_elem_f *e_prev = 0;

	// Look for teh element and destroy if found
	while (e) {
		
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		

		if ( memcmp(e->key, key, key_len) == 0) {
			// Found it, kill it
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
					memset(e, 0, sizeof(rchash_elem_f));
				}
				// at head with a next - more complicated
				else {
					rchash_elem_f *_t = e->next;
					memcpy(e, e->next, sizeof(rchash_elem_f)+key_len);
					free(_t);
				}
			}
			
			if (h->flags & RCHASH_CR_MT_MANYLOCK)
				cf_atomic32_decr(&h->elements);
			else
				h->elements--;
			
			rv = RCHASH_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = RCHASH_ERR_NOTFOUND;

Out:
	if (l)	pthread_mutex_unlock(l);
	return(rv);	
	

}

// Call the function over every node in the tree
// Can be lock-expensive at the moment, until we improve the lockfree code

void
rchash_reduce(rchash *h, rchash_reduce_fn reduce_fn, void *udata)
{
    if (h->key_len == 0)    {
        rchash_reduce_v(h,reduce_fn, udata);
        return;
    }
	
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);
	
	for (uint i=0; i<h->table_len ; i++) {

		pthread_mutex_t *l = 0;
		if (h->flags & RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}

		rchash_elem_f *list_he = get_bucket(h, i);	

		while (list_he) {
			
			// 0 length means an unused head pointer - break
			if (list_he->object == 0)
				break;
			
#ifdef VALIDATE
			if (cf_rc_count(list_he->object) < 1) {
				cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, list_he->object);
			}
#endif		

			if (0 != reduce_fn(list_he->key, h->key_len, list_he->object, udata)) {
				if (l)		pthread_mutex_unlock(l);
				goto Out;
			}
			
			list_he = list_he->next;
		};
		
		if (l)	pthread_mutex_unlock(l);
		
	}

Out:	
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
	if (h->key_len == 0) {
        rchash_reduce_delete_v(h,reduce_fn, udata);
        return;
    }
    
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	for (uint i=0; i<h->table_len ; i++) {

		pthread_mutex_t *l = 0;
		if (h->flags & RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}
		
		rchash_elem_f *list_he = get_bucket(h, i);
		rchash_elem_f *prev_he = 0;
		int rv;

		while (list_he) {
			// This kind of structure might have the head as an empty element,
			// that's a signal to move along
			if (list_he->object == 0)
				break;
			
#ifdef VALIDATE
			if (cf_rc_count(list_he->object) < 1) {
				cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, list_he->object);
				if (l)	pthread_mutex_unlock(l);
				return;
			}
#endif		
			
			rv = reduce_fn(list_he->key, h->key_len, list_he->object, udata);
			
			// Delete is requested
			// Leave the pointers in a "next" state
			if (rv == RCHASH_REDUCE_DELETE) {
                
				rchash_free(h, list_he->object);
				
				if (h->flags & RCHASH_CR_MT_MANYLOCK)
					cf_atomic32_decr(&h->elements);
				else
					h->elements--;
                
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
						memset(list_he, 0, sizeof(rchash_elem_f));
						list_he = 0;
					}
					// at head with a next - more complicated -
					// copy next into current and free next
					// (the old trick of how to delete from a singly
					// linked list without a prev pointer)
					// Somewhat confusingly, prev_he stays 0
					// and list_he stays where it is
					else {
						rchash_elem_f *_t = list_he->next;
						memcpy(list_he, list_he->next, sizeof(rchash_elem_f)+h->key_len);
						free(_t);
					}
				}

			}
			else { // don't delete, just forward everything
				prev_he = list_he;
				list_he = list_he->next;
			}	
				
		};
		
		if (l) pthread_mutex_unlock(l);
		
	}

	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return;
}

void
rchash_destroy_elements(rchash *h)
{
	for (uint i=0;i<h->table_len;i++) {
        rchash_elem_f *e = get_bucket(h, i);
        if (e->object == 0) continue;
        rchash_free(h, e->object);
        e = e->next; // skip the first, it's in place

        while (e) {
            rchash_elem_f *t = e->next;
            rchash_free(h, e->object);
            free(e);
            e = t;
		}
	}
	h->elements = 0;
}

void
rchash_destroy(rchash *h)
{
    if (h->key_len == 0) rchash_destroy_elements_v(h);
    else                 rchash_destroy_elements(h);

	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_destroy(&h->biglock);
	}
	if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		for (uint i=0;i<h->table_len;i++) {
			pthread_mutex_destroy(&(h->lock_table[i]));
		}
		free(h->lock_table);
	}

	free(h->table);
	free(h);	
}	

inline static
rchash_elem_v *
get_bucket_v(rchash *h, uint i)
{
    return ( (rchash_elem_v *) 
               ( 
                 ((uint8_t *)h->table) + (sizeof(rchash_elem_v) * i)
               ) 
           );
}


uint32_t
rchash_get_size_v(rchash *h)
{
    uint32_t sz = 0;
    
    if (h->flags & RCHASH_CR_MT_BIGLOCK) {
    	pthread_mutex_lock(&h->biglock);
    	sz = h->elements;
    	pthread_mutex_unlock(&h->biglock);
    }
    else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
    	sz = cf_atomic32_get(h->elements);
    }
    else {
    	sz = h->elements;
    }
    return(sz);
}


int
rchash_put_v(rchash *h, void *key, uint32_t key_len, void *object)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	rchash_elem_v *e = get_bucket_v(h, hash);	

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0 ) 
		goto Copy;

	rchash_elem_v *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)	
	while (e) {
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)		pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		
        // in this case we're replacing the previous object with the new object
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			rchash_free(h,e->object);
			e->object = object;
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_OK);
		}
		e = e->next;
	}

	e = (rchash_elem_v *) CF_MALLOC(sizeof(rchash_elem_v));
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = CF_MALLOC(key_len);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;

	e->object = object;

	h->elements++;
	if (l)		pthread_mutex_unlock(l);
	return(RCHASH_OK);	

}

//
// Put of any sort gobbles the reference count.
// make sure the incoming reference count is > 0
//

int
rchash_put_unique_v(rchash *h, void *key, uint32_t key_len, void *object)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(RCHASH_ERR);

#ifdef VALIDATE
	if (cf_rc_count(object) < 1) {
		cf_info(CF_RCHASH,"put unique! bad reference count on %p");
		return(RCHASH_ERR);
	}
#endif    
	
	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	rchash_elem_v *e = get_bucket_v(h, hash);	

	// most common case should be insert into empty bucket, special case
	if ( e->object ) 
		goto Copy;

	rchash_elem_v *e_head = e;

	// check for uniqueness of key - if not unique, fail!
	while (e) {
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			if (l) pthread_mutex_unlock(l);
			return(RCHASH_ERR_FOUND);
		}
		e = e->next;
	}

	e = (rchash_elem_v *) CF_MALLOC(sizeof(rchash_elem_v));
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = CF_MALLOC(key_len);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;

	e->object = object;

	h->elements++;
	if (l)		pthread_mutex_unlock(l);
	return(RCHASH_OK);	

}



int
rchash_get_v(rchash *h, void *key, uint32_t key_len, void **object)
{
	int rv = RCHASH_ERR;
	
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
	
	rchash_elem_v *e = get_bucket_v(h, hash);	

  	// most common case should be insert into empty bucket, special case
	if ( e->object ) {
        rv = RCHASH_ERR_NOTFOUND;
        goto Out;
	}
    
	while (e) {
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_ERR);
		}
#endif		

		if ( ( key_len == e->key_len ) &&
			 ( memcmp(key, e->key, key_len) == 0) ) {
			cf_rc_reserve( e->object );
			*object = e->object;
			rv = RCHASH_OK; 
			goto Out;
		}
		e = e->next;
	};
    
	rv = RCHASH_ERR_NOTFOUND;
	
Out:
	if (l)
		pthread_mutex_unlock(l);

	return(rv);
					
}

int
rchash_delete_v(rchash *h, void *key, uint32_t key_len)
{
	if ((h->key_len) &&  (h->key_len != key_len) ) return(RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = RCHASH_ERR;

	pthread_mutex_t		*l = 0;
	if (h->flags & RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	rchash_elem_v *e = get_bucket_v(h, hash);	

	// If bucket empty, def can't delete
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		rv = RCHASH_ERR_NOTFOUND;
		goto Out;
	}

	rchash_elem_v *e_prev = 0;

	// Look for teh element and destroy if found
	while (e) {
		
#ifdef VALIDATE
		if (cf_rc_count(e->object) < 1) {
			cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(RCHASH_ERR);
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
					memset(e, 0, sizeof(rchash_elem_v));
				}
				// at head with a next - more complicated
				else {
					rchash_elem_v *_t = e->next;
					memcpy(e, e->next, sizeof(rchash_elem_v));
					free(_t);
				}
			}
			h->elements--;
			rv = RCHASH_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = RCHASH_ERR_NOTFOUND;

Out:
	if (l)	pthread_mutex_unlock(l);
	return(rv);	
	

}

// Call the function over every node in the tree
// Can be lock-expensive at the moment, until we improve the lockfree code

void
rchash_reduce_v(rchash *h, rchash_reduce_fn reduce_fn, void *udata)
{
	
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);
	
	for (uint i=0; i<h->table_len ; i++) {

		pthread_mutex_t *l = 0;
		if (h->flags & RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}

		rchash_elem_v *list_he = get_bucket_v(h, i);	

		while (list_he) {
			
			// 0 length means an unused head pointer - break
			if (list_he->object == 0)
				break;
			
#ifdef VALIDATE
			if (cf_rc_count(list_he->object) < 1) {
				cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, list_he->object);
			}
#endif		

			if (0 != reduce_fn(list_he->key, list_he->key_len, list_he->object, udata)) {
				if (l)		pthread_mutex_unlock(l);
				goto Out;
			}
			
			list_he = list_he->next;
		};
		
		if (l)	pthread_mutex_unlock(l);
		
	}

Out:	
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return;
}

// A special version of 'reduce' that supports deletion
// In this case, if you return '-1' from the reduce fn, that node will be
// deleted
void
rchash_reduce_delete_v(rchash *h, rchash_reduce_fn reduce_fn, void *udata)
{
	
	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	for (uint i=0; i<h->table_len ; i++) {

		pthread_mutex_t *l = 0;
		if (h->flags & RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock( l );
		}
		
		rchash_elem_v *list_he = get_bucket_v(h, i);
		rchash_elem_v *prev_he = 0;
		int rv;

		while (list_he) {
			// This kind of structure might have the head as an empty element,
			// that's a signal to move along
			if (list_he->key_len == 0)
				break;
			
#ifdef VALIDATE
			if (cf_rc_count(list_he->object) < 1) {
				cf_info(CF_RCHASH,"rchash %p: internal bad reference count on %p",h, list_he->object);
				if (l)	pthread_mutex_unlock(l);
				return;
			}
#endif		
			
			rv = reduce_fn(list_he->key, list_he->key_len, list_he->object, udata);
			
			// Delete is requested
			// Leave the pointers in a "next" state
			if (rv == RCHASH_REDUCE_DELETE) {
                
				free(list_he->key);
				rchash_free(h, list_he->object);
                h->elements--;
                
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
						memset(list_he, 0, sizeof(rchash_elem_v));
						list_he = 0;
					}
					// at head with a next - more complicated -
					// copy next into current and free next
					// (the old trick of how to delete from a singly
					// linked list without a prev pointer)
					// Somewhat confusingly, prev_he stays 0
					// and list_he stays where it is
					else {
						rchash_elem_v *_t = list_he->next;
						memcpy(list_he, list_he->next, sizeof(rchash_elem_v));
						free(_t);
					}
				}

			}
			else { // don't delete, just forward everything
				prev_he = list_he;
				list_he = list_he->next;
			}	
				
		};
		
		if (l) pthread_mutex_unlock(l);
		
	}

	if (h->flags & RCHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return;
}


void
rchash_destroy_elements_v(rchash *h)
{
	for (uint i=0;i<h->table_len;i++) {
        
        rchash_elem_f *e = get_bucket(h, i);
        if (e->object == 0) continue;
        
        rchash_free(h, e->object);
        free(e->key);
        e = e->next; // skip the first, it's in place

        while (e) {
            rchash_elem_f *t = e->next;
            rchash_free(h, e->object);
            free(e->key);
            free(e);
            e = t;
		}
	}

}	
