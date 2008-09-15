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
bbhash_create(bbhash **h_r, bbhash_hash_fn h_fn, int key_len, int value_len, uint sz, uint flags)
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

int
bbhash_put(bbhash *h, void *key, int key_len, void *value, int value_len)
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
	if (h->flags & BBHASH_CR_MT_BIGLOCK) 
		pthread_mutex_unlock(&h->biglock);
	return(BB_OK);	

}

int
bbhash_get(bbhash *h, void *key, int key_len, void *value, int *value_len)
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

// Call the function over every node in the tree
// Can be lock-expensive at the moment, until we improve the lockfree code

void
bbhash_reduce(bbhash *h, bbhash_reduce_fn reduce_fn, void *udata)
{
	
	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_lock(&h->biglock);

	bbhash_elem *he = h->table;
	
	for (uint i=0; i<h->table_len ; i++) {

		bbhash_elem *list_he = he;
		while (list_he) {
			reduce_fn(he->key, he->key_len, he->value, he->value_len, udata);
			list_he = list_he->next;
		};
		
		he++;
	}

	if (h->flags & BBHASH_CR_MT_BIGLOCK)
		pthread_mutex_unlock(&h->biglock);

	return;
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
