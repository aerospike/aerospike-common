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
 * Which supports the citrusleaf reference counting
 * natively
 *
 * You can only put a pointer in. Having the pointer in the table holds
 * its reference count. Doing a delete decreases the reference count
 * internally. As ... ?????
 *
 * Just, hopefully, the last reasonable hash table you'll ever need 
 * ... that's what he said about shash!
 */

#include <citrusleaf/cf_rchash.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <citrusleaf/alloc.h>
#include <citrusleaf/cf_atomic.h>

/******************************************************************************
 * FUNCTION DECLS
 ******************************************************************************/

void cf_rchash_destroy_v(cf_rchash *h);
void cf_rchash_reduce_delete_v(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata);
int cf_rchash_reduce_v(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata);
int cf_rchash_delete_v(cf_rchash *h, void *key, uint32_t key_len);
int cf_rchash_get_v(cf_rchash *h, void *key, uint32_t key_len, void **object);
int cf_rchash_put_unique_v(cf_rchash *h, void *key, uint32_t key_len, void *object);
int cf_rchash_put_v(cf_rchash *h, void *key, uint32_t key_len, void *object);
uint32_t cf_rchash_get_size_v(cf_rchash *h);
void cf_rchash_destroy_elements_v(cf_rchash *h);
void cf_rchash_dump_v(cf_rchash *h);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int cf_rchash_create(cf_rchash **h_r, cf_rchash_hash_fn h_fn, cf_rchash_destructor_fn d_fn, uint32_t key_len, uint32_t sz, uint flags) {
	cf_rchash *h;

	h = cf_malloc(sizeof(cf_rchash));
	if (!h)	return(CF_RCHASH_ERR);

	h->elements = 0;
	h->table_len = sz;
	h->key_len = key_len;
	h->flags = flags;
	h->h_fn = h_fn;
	h->d_fn = d_fn;

	if ((flags & CF_RCHASH_CR_MT_BIGLOCK) && (flags & CF_RCHASH_CR_MT_MANYLOCK)) {
		*h_r = 0;
		return(CF_RCHASH_ERR);
	}

	if (key_len == 0)
        h->table = cf_calloc(sz, sizeof(cf_rchash_elem_v));
    else
        h->table = cf_calloc(sz, sizeof(cf_rchash_elem_f) + key_len);
    
	if (!h->table) {
		cf_free(h);
		return(CF_RCHASH_ERR);
	}

	if (flags & CF_RCHASH_CR_MT_BIGLOCK) {
		if (0 != pthread_mutex_init ( &h->biglock, 0) ) {
			cf_free(h->table);
			cf_free(h);
			return(CF_RCHASH_ERR);
		}
	}
	else
		memset( &h->biglock, 0, sizeof( h->biglock ) );
	
	if (flags & CF_RCHASH_CR_MT_MANYLOCK) {
		h->lock_table = cf_malloc( sizeof(pthread_mutex_t) * sz);
		if (! h->lock_table) {
			cf_free(h);
			*h_r = 0;
			return(CF_RCHASH_ERR);
		}
		for (uint i=0;i<sz;i++) {
			pthread_mutex_init( &(h->lock_table[i]), 0 );
		}
	}
	else
		h->lock_table = 0;

	*h_r = h;

	return(CF_RCHASH_OK);
}

void cf_rchash_free(cf_rchash *h, void *object) {
	if (cf_rc_release(object) == 0) {
		if (h->d_fn)	(h->d_fn) (object) ;
		cf_rc_free(object);
	}
}

static inline cf_rchash_elem_f *get_bucket(cf_rchash *h, uint i) {
    return( (cf_rchash_elem_f * ) (
                ((uint8_t *) h->table) +
                ((sizeof(cf_rchash_elem_f) + h->key_len) * i) 
             )
           );
}

uint32_t cf_rchash_get_size(cf_rchash *h) {
    if (h->key_len == 0)    return(cf_rchash_get_size_v(h));
    
    uint32_t sz = 0;
    
    if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
    	pthread_mutex_lock(&h->biglock);
    	sz = h->elements;
    	pthread_mutex_unlock(&h->biglock);
    }
    else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
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

		cf_rchash_elem_f *list_he = get_bucket(h, i);

		while (list_he) {
			// null object means unused head pointer
			if (list_he->object == 0)
				break;
			validate_size++;
			list_he = list_he->next;
		}
		
		pthread_mutex_unlock(l);
		
	}
#endif

    return(sz);
    
}

int cf_rchash_put(cf_rchash *h, void *key, uint32_t key_len, void *object) {
    if (h->key_len == 0)    return(cf_rchash_put_v(h, key, key_len, object));

	if (h->key_len != key_len) return(CF_RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	cf_rchash_elem_f *e = get_bucket(h, hash);

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0  ) {
		goto Copy;
	}

	cf_rchash_elem_f *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)
	while (e) {
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)		pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		
        // in this case we're replacing the previous object with the new object
		if ( memcmp(e->key, key, key_len) == 0) {
			cf_rchash_free(h,e->object);
			e->object = object;
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_OK);
		}
		e = e->next;
	}

	e = (cf_rchash_elem_f *) cf_malloc(sizeof(cf_rchash_elem_f) + key_len);
	if (!e) return (CF_RCHASH_ERR);
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(e->key, key, key_len);

	e->object = object;

	if (h->flags & CF_RCHASH_CR_MT_MANYLOCK)
		cf_atomic32_incr(&h->elements);
	else
		h->elements++;
	
	if (l)		pthread_mutex_unlock(l);
	return(CF_RCHASH_OK);
}

//
// Put of any sort gobbles the reference count.
// make sure the incoming reference count is > 0
//

int cf_rchash_put_unique(cf_rchash *h, void *key, uint32_t key_len, void *object) {
    if (h->key_len == 0)    return(cf_rchash_put_unique_v(h,key,key_len,object));

	if (h->key_len != key_len) return(CF_RCHASH_ERR);

#ifdef VALIDATE
	cf_atomic_int_t rc;
	if ((rc = cf_rc_count(object)) < 1) {
		as_log_info("cf_rchash %d: put unique! bad reference count (%d) on %p", h, rc, object);
		return(CF_RCHASH_ERR);
	}
#endif    
	
	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	cf_rchash_elem_f *e = get_bucket(h, hash);

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0 ) goto Copy;

	cf_rchash_elem_f *e_head = e;

	// check for uniqueness of key - if not unique, fail!
	while (e) {
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		
		if ( memcmp(e->key, key, key_len) == 0) {
			if (l) pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR_FOUND);
		}
		e = e->next;
	}

	e = (cf_rchash_elem_f *) cf_malloc(sizeof(cf_rchash_elem_f) + key_len);
	if (!e) return (CF_RCHASH_ERR);
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	memcpy(e->key, key, key_len);

	e->object = object;

	if (h->flags & CF_RCHASH_CR_MT_MANYLOCK)
		cf_atomic32_incr(&h->elements);
	else
		h->elements++;

	if (l)		pthread_mutex_unlock(l);
	return(CF_RCHASH_OK);
}

int cf_rchash_get(cf_rchash *h, void *key, uint32_t key_len, void **object) {
	if (!h || !key) return(CF_RCHASH_ERR);
    if (h->key_len == 0)    return(cf_rchash_get_v(h,key,key_len,object));
	if (h->key_len != key_len) return(CF_RCHASH_ERR);

	int rv = CF_RCHASH_ERR;
	
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
	
	cf_rchash_elem_f *e = get_bucket(h, hash);

  	// finding an empty bucket means key is not here
	if ( e->object == 0 ) {
        rv = CF_RCHASH_ERR_NOTFOUND;
        goto Out;
	}
    
	while (e) {
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif

		if ( memcmp(key, e->key, key_len) == 0) {
			if (object) {
				cf_rc_reserve( e->object );
				*object = e->object;
			}
			rv = CF_RCHASH_OK; 
			goto Out;
		}
		e = e->next;
	}
    
	rv = CF_RCHASH_ERR_NOTFOUND;
	
Out:
	if (l)
		pthread_mutex_unlock(l);

	return(rv);
}

int cf_rchash_delete(cf_rchash *h, void *key, uint32_t key_len) {
    if (h->key_len == 0)    return(cf_rchash_delete_v(h,key,key_len));
	if (h->key_len != key_len) return(CF_RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = CF_RCHASH_ERR;

    // take lock
	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	cf_rchash_elem_f *e = get_bucket(h, hash);

	// If bucket empty, def can't delete
	if ( e->object == 0 ) {
		rv = CF_RCHASH_ERR_NOTFOUND;
		goto Out;
	}

	cf_rchash_elem_f *e_prev = 0;

	// Look for the element and destroy if found
	while (e) {
		
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		

		if ( memcmp(e->key, key, key_len) == 0) {
			// Found it, kill it
			cf_rchash_free(h, e->object);
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				cf_free(e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					memset(e, 0, sizeof(cf_rchash_elem_f));
				}
				// at head with a next - more complicated
				else {
					cf_rchash_elem_f *_t = e->next;
					memcpy(e, e->next, sizeof(cf_rchash_elem_f)+key_len);
					cf_free(_t);
				}
			}
			
			if (h->flags & CF_RCHASH_CR_MT_MANYLOCK)
				cf_atomic32_decr(&h->elements);
			else
				h->elements--;
			
			rv = CF_RCHASH_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = CF_RCHASH_ERR_NOTFOUND;

Out:
	if (l)	pthread_mutex_unlock(l);
	return(rv);
}

// Call the given function (reduce_fn) over every node in the tree.
// Can be lock-expensive at the moment, until we improve the lockfree code.
// The value returned by reduce_fn governs behavior as follows:
//     CF_RCHASH_OK (0) - continue iterating.
//     CF_RCHASH_REDUCE_DELETE (1) - delete the current node.
//     Anything else (e.g. CF_RCHASH_ERR) - stop iterating and return
//         reduce_fn's returned value.

int cf_rchash_reduce(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata) {
	if (h->key_len == 0) {
		return cf_rchash_reduce_v(h, reduce_fn, udata);
	}

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}

	for (uint32_t i = 0; i < h->table_len; i++) {
		pthread_mutex_t *l = NULL;

		if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock(l);
		}

		cf_rchash_elem_f *list_he = get_bucket(h, i);
		cf_rchash_elem_f *prev_he = NULL;

		while (list_he) {
			if (! list_he->object) {
				// Nothing (more) in this row.
				break;
			}

#ifdef VALIDATE
			cf_atomic_int_t rc;
			if ((rc = cf_rc_count(list_he->object)) < 1) {
				as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, list_he->object);
				if (l)	pthread_mutex_unlock(l);
				return;
			}
#endif		

			int rv = reduce_fn(list_he->key, h->key_len, list_he->object, udata);

			if (rv == CF_RCHASH_OK) {
				// Most common case - keep going.
				prev_he = list_he;
				list_he = list_he->next;
			}
			else if (rv == CF_RCHASH_REDUCE_DELETE) {
				cf_rchash_free(h, list_he->object);

				if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
					cf_atomic32_decr(&h->elements);
				}
				else {
					h->elements--;
				}

				if (prev_he) {
					prev_he->next = list_he->next;
					cf_free(list_he);
					list_he = prev_he->next;
				}
				else {
					// At head with no next.
					if (! list_he->next) {
						memset(list_he, 0, sizeof(cf_rchash_elem_f));
						list_he = NULL;
					}
					// At head with a next. Copy next into current and free
					// next. prev_he stays NULL, list_he stays unchanged.
					else {
						cf_rchash_elem_f *_t = list_he->next;
						memcpy(list_he, list_he->next, sizeof(cf_rchash_elem_f) + h->key_len);
						cf_free(_t);
					}
				}
			}
			else {
				// Stop iterating.
				if (l) {
					pthread_mutex_unlock(l);
				}

				if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
					pthread_mutex_unlock(&h->biglock);
				}

				return rv;
			}
		}

		if (l) {
			pthread_mutex_unlock(l);
		}
	}

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_unlock(&h->biglock);
	}

	return CF_RCHASH_OK;
}

void cf_rchash_destroy_elements(cf_rchash *h) {
	for (uint i=0;i<h->table_len;i++) {
        cf_rchash_elem_f *e = get_bucket(h, i);
        if (e->object == 0) continue;
        cf_rchash_free(h, e->object);
        e = e->next; // skip the first, it's in place

        while (e) {
            cf_rchash_elem_f *t = e->next;
            cf_rchash_free(h, e->object);
            cf_free(e);
            e = t;
		}
	}
	h->elements = 0;
}

void cf_rchash_destroy(cf_rchash *h) {
    if (h->key_len == 0) cf_rchash_destroy_elements_v(h);
    else                 cf_rchash_destroy_elements(h);

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_destroy(&h->biglock);
	}
	if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		for (uint i=0;i<h->table_len;i++) {
			pthread_mutex_destroy(&(h->lock_table[i]));
		}
		cf_free(h->lock_table);
	}

	cf_free(h->table);
	cf_free(h);
}

inline static cf_rchash_elem_v * get_bucket_v(cf_rchash *h, uint i) {
    return ( (cf_rchash_elem_v *) 
               ( 
                 ((uint8_t *)h->table) + (sizeof(cf_rchash_elem_v) * i)
               ) 
           );
}


uint32_t cf_rchash_get_size_v(cf_rchash *h) {
    uint32_t sz = 0;
    
    if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
    	pthread_mutex_lock(&h->biglock);
    	sz = h->elements;
    	pthread_mutex_unlock(&h->biglock);
    }
    else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
    	sz = cf_atomic32_get(h->elements);
    }
    else {
    	sz = h->elements;
    }
    return(sz);
}


int cf_rchash_put_v(cf_rchash *h, void *key, uint32_t key_len, void *object) {
	if ((h->key_len) &&  (h->key_len != key_len) ) return(CF_RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	cf_rchash_elem_v *e = get_bucket_v(h, hash);

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0 )
		goto Copy;

	cf_rchash_elem_v *e_head = e;

	// This loop might be skippable if you know the key is not already in the hash
	// (like, you just searched and it's single-threaded)
	while (e) {
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count on %p", h, rc, e->object);
			if (l)		pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		
        // in this case we're replacing the previous object with the new object
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			cf_rchash_free(h,e->object);
			e->object = object;
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_OK);
		}
		e = e->next;
	}

	e = (cf_rchash_elem_v *) cf_malloc(sizeof(cf_rchash_elem_v));
	if (!e)	return (CF_RCHASH_ERR);
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = cf_malloc(key_len);
	if (!e->key) return (CF_RCHASH_ERR);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;

	e->object = object;

	h->elements++;
	if (l)		pthread_mutex_unlock(l);
	return(CF_RCHASH_OK);
}

//
// Put of any sort gobbles the reference count.
// make sure the incoming reference count is > 0
//

int cf_rchash_put_unique_v(cf_rchash *h, void *key, uint32_t key_len, void *object) {
	if ((h->key_len) &&  (h->key_len != key_len) ) return(CF_RCHASH_ERR);

#ifdef VALIDATE
	cf_atomic_int_t rc;
	if ((rc = cf_rc_count(object)) < 1) {
		as_log_info("cf_rchash %p: put unique! bad reference count (%d) on %p", h, rc, object);
		return(CF_RCHASH_ERR);
	}
#endif    
	
	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	cf_rchash_elem_v *e = get_bucket_v(h, hash);

	// most common case should be insert into empty bucket, special case
	if ( e->object == 0 )
		goto Copy;

	cf_rchash_elem_v *e_head = e;

	// check for uniqueness of key - if not unique, fail!
	while (e) {
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		
		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			if (l) pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR_FOUND);
		}
		e = e->next;
	}

	e = (cf_rchash_elem_v *) cf_malloc(sizeof(cf_rchash_elem_v));
	if (!e)	return (CF_RCHASH_ERR);
	e->next = e_head->next;
	e_head->next = e;
	
Copy:
	e->key = cf_malloc(key_len);
	if (!e->key) return (CF_RCHASH_ERR);
	memcpy(e->key, key, key_len);
	e->key_len = key_len;

	e->object = object;

	h->elements++;
	if (l)		pthread_mutex_unlock(l);
	return(CF_RCHASH_OK);
}

int cf_rchash_get_v(cf_rchash *h, void *key, uint32_t key_len, void **object) {
	int rv = CF_RCHASH_ERR;
	
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
	
	cf_rchash_elem_v *e = get_bucket_v(h, hash);

  	// finding an empty bucket means key is not here
	if ( e->object == 0 ) {
        rv = CF_RCHASH_ERR_NOTFOUND;
        goto Out;
	}
    
	while (e) {
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		

		if ( ( key_len == e->key_len ) &&
			 ( memcmp(key, e->key, key_len) == 0) ) {
			if (object) {
				cf_rc_reserve( e->object );
				*object = e->object;
			}
			rv = CF_RCHASH_OK; 
			goto Out;
		}
		e = e->next;
	}
    
	rv = CF_RCHASH_ERR_NOTFOUND;
	
Out:
	if (l)
		pthread_mutex_unlock(l);

	return(rv);
}

int cf_rchash_delete_v(cf_rchash *h, void *key, uint32_t key_len) {
	if ((h->key_len) &&  (h->key_len != key_len) ) return(CF_RCHASH_ERR);

	// Calculate hash
	uint hash = h->h_fn(key, key_len);
	hash %= h->table_len;
	int rv = CF_RCHASH_ERR;

	pthread_mutex_t		*l = 0;
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		l = &h->biglock;
	}
	else if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
		l = & (h->lock_table[hash]);
	}
	if (l)     pthread_mutex_lock( l );
		
	cf_rchash_elem_v *e = get_bucket_v(h, hash);

	// If bucket empty, def can't delete
	if ( ( e->next == 0 ) && (e->key_len == 0) ) {
		rv = CF_RCHASH_ERR_NOTFOUND;
		goto Out;
	}

	cf_rchash_elem_v *e_prev = 0;

	// Look for the element and destroy if found
	while (e) {
		
#ifdef VALIDATE
		cf_atomic_int_t rc;
		if ((rc = cf_rc_count(e->object)) < 1) {
			as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, e->object);
			if (l)	pthread_mutex_unlock(l);
			return(CF_RCHASH_ERR);
		}
#endif		

		if ( ( key_len == e->key_len ) &&
			 ( memcmp(e->key, key, key_len) == 0) ) {
			// Found it, kill it
			cf_free(e->key);
			cf_rchash_free(h, e->object);
			// patchup pointers & free element if not head
			if (e_prev) {
				e_prev->next = e->next;
				cf_free(e);
			}
			// am at head - more complicated
			else {
				// at head with no next - easy peasy!
				if (0 == e->next) {
					memset(e, 0, sizeof(cf_rchash_elem_v));
				}
				// at head with a next - more complicated
				else {
					cf_rchash_elem_v *_t = e->next;
					memcpy(e, e->next, sizeof(cf_rchash_elem_v));
					cf_free(_t);
				}
			}
			h->elements--;
			rv = CF_RCHASH_OK;
			goto Out;

		}
		e_prev = e;
		e = e->next;
	}
	rv = CF_RCHASH_ERR_NOTFOUND;

Out:
	if (l)	pthread_mutex_unlock(l);
	return(rv);
}

// Call the given function (reduce_fn) over every node in the tree.
// Can be lock-expensive at the moment, until we improve the lockfree code.
// The value returned by reduce_fn governs behavior as follows:
//     CF_RCHASH_OK (0) - continue iterating.
//     CF_RCHASH_REDUCE_DELETE (1) - delete the current node.
//     Anything else (e.g. CF_RCHASH_ERR) - stop iterating and return
//         reduce_fn's returned value.

int cf_rchash_reduce_v(cf_rchash *h, cf_rchash_reduce_fn reduce_fn, void *udata) {

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}

	for (uint32_t i = 0; i < h->table_len; i++) {
		pthread_mutex_t *l = NULL;

		if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock(l);
		}

		cf_rchash_elem_v *list_he = get_bucket_v(h, i);
		cf_rchash_elem_v *prev_he = NULL;

		while (list_he) {
			if (list_he->key_len == 0) {
				// Nothing (more) in this row.
				break;
			}

#ifdef VALIDATE
			cf_atomic_int_t rc;
			if ((rc = cf_rc_count(list_he->object)) < 1) {
				as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, list_he->object);
				if (l)	pthread_mutex_unlock(l);
				return;
			}
#endif		

			int rv = reduce_fn(list_he->key, list_he->key_len, list_he->object, udata);

			if (rv == CF_RCHASH_OK) {
				// Most common case - keep going.
				prev_he = list_he;
				list_he = list_he->next;
			}
			else if (rv == CF_RCHASH_REDUCE_DELETE) {
				cf_free(list_he->key);
				cf_rchash_free(h, list_he->object);

				if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
					cf_atomic32_decr(&h->elements);
				}
				else {
					h->elements--;
				}

				if (prev_he) {
					prev_he->next = list_he->next;
					cf_free(list_he);
					list_he = prev_he->next;
				}
				else {
					// At head with no next.
					if (! list_he->next) {
						memset(list_he, 0, sizeof(cf_rchash_elem_v));
						list_he = NULL;
					}
					// At head with a next. Copy next into current and free
					// next. prev_he stays NULL, list_he stays unchanged.
					else {
						cf_rchash_elem_v *_t = list_he->next;
						memcpy(list_he, list_he->next, sizeof(cf_rchash_elem_v));
						cf_free(_t);
					}
				}
			}
			else {
				// Stop iterating.
				if (l) {
					pthread_mutex_unlock(l);
				}

				if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
					pthread_mutex_unlock(&h->biglock);
				}

				return rv;
			}
		}

		if (l) {
			pthread_mutex_unlock(l);
		}
	}

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_unlock(&h->biglock);
	}

	return CF_RCHASH_OK;
}

void cf_rchash_destroy_elements_v(cf_rchash *h) {
	for (uint i=0;i<h->table_len;i++) {
        
        cf_rchash_elem_v *e = get_bucket_v(h, i);
        if (e->object == 0) continue;
        
        cf_rchash_free(h, e->object);
        cf_free(e->key);
        e = e->next; // skip the first, it's in place

        while (e) {
            cf_rchash_elem_v *t = e->next;
            cf_rchash_free(h, e->object);
            cf_free(e->key);
            cf_free(e);
            e = t;
		}
	}
}

#ifdef DEBUG_HASH

/*
 *  Print contents of an rchash table.
 */
void cf_rchash_dump(cf_rchash *h)
{
	if (!(h->key_len)) {
		cf_rchash_dump_v(h);
		return;
	}

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}

	as_log_info("rchash: %p ; flags 0x%08x ; key_len %d ; table_len %d", h, h->flags, h->key_len, h->table_len);
	if (!(h->flags & CF_RCHASH_CR_MT_MANYLOCK)) {
		as_log_info("number of elements: %d", h->elements);
	}

	as_log_info("Elements:");
	as_log_info("---------");

	for (uint i = 0; i < h->table_len; i++) {
		pthread_mutex_t *l = 0;
		if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock(l);
		}

		cf_rchash_elem_f *list_he = get_bucket(h, i);

		uint j = 0;
		while (list_he) {
			// 0 length means an unused head pointer - break
			if (list_he->object == 0)
			  break;
			
			// (Always validates.)
			cf_atomic_int_t rc;
			if ((rc = cf_rc_count(list_he->object)) < 1) {
				as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, list_he->object);
			}

			as_log_info("[%d.%d] key: %p ; object: %p (rc %d)", i, j++, list_he->key, list_he->object, rc);

			list_he = list_he->next;
		}

		if (l)	pthread_mutex_unlock(l);
	}

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_unlock(&h->biglock);
	}
}

/*
 *  Print contents of a variable-key-length rchash table.
 */
void cf_rchash_dump_v(cf_rchash *h)
{
	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_lock(&h->biglock);
	}

	as_log_info("rchash: %p ; flags 0x%08x ; key_len %d ; table_len %d", h, h->flags, h->key_len, h->table_len);
	if (!(h->flags & CF_RCHASH_CR_MT_MANYLOCK)) {
		as_log_info("number of elements: %d", h->elements);
	}

	as_log_info("Elements:");
	as_log_info("---------");

	for (uint i = 0; i < h->table_len; i++) {
		pthread_mutex_t *l = 0;
		if (h->flags & CF_RCHASH_CR_MT_MANYLOCK) {
			l = &(h->lock_table[i]);
			pthread_mutex_lock(l);
		}

		cf_rchash_elem_v *list_he = get_bucket_v(h, i);

		uint j = 0;
		while (list_he) {
			// 0 length means an unused head pointer - break
			if (list_he->object == 0)
			  break;

			// (Always validates.)
			cf_atomic_int_t rc;
			if ((rc = cf_rc_count(list_he->object)) < 1) {
				as_log_info("cf_rchash %p: internal bad reference count (%d) on %p", h, rc, list_he->object);
			}

			as_log_info("[%d.%d] key: %p ; key_len: %d ; object: %p (rc %d)", i, j++, list_he->key, list_he->key_len, list_he->object, rc);

			// XXX -- Not general ~~ Requires a string key.
			// (Should pass in a printing function instead.)
			as_log_info("key: \"%s\"", (char *) list_he->key);

			list_he = list_he->next;
		}

		if (l)	pthread_mutex_unlock(l);
	}

	if (h->flags & CF_RCHASH_CR_MT_BIGLOCK) {
		pthread_mutex_unlock(&h->biglock);
	}
}

#endif // defined(DEBUG)
