/*
 * A general purpose hashtable implementation
 * Good at multithreading
 * Just, hopefully, the last reasonable hash table you'll ever need
 * Copywrite 2008 Brian Bulkowski
 * All rights reserved
 */

#include <string.h>
#include <stdlib.h>


#include "vector.h"



int
vector_create(vector **v_r, uint32_t value_len, uint32_t init_sz, uint flags)
{
	vector *v;

	v = malloc(sizeof(vector));
	if (!v)	return(SHASH_ERR);

	v->value_len = value_len;
	v->flags = flags;
	v->alloc_len = init_sz;
	v->len = 0;
	if (init_sz)
		v->vector = malloc(init_sz * value_len);
	else
		v->vector = 0;
	if (flags & VECTOR_FLAG_INITZERO)
		memset(v->vector, 0, init_sz * value_len);
	if (flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_init(&v->LOCK, 0);
	
	return(SHASH_OK);
}

static int
vector_resize(vector *v, uint32_t new_sz)
{
	if (v->flags & VECTOR_FLAG_BIGRESIZE) {
		if (new_sz < 50)	new_sz = 50;
		else if (new_sz < v->alloc_len * 2)
			new_sz = v->alloc_len * 2;
	}
	uint8_t	*_t = realloc(v->vector, (new_sz) * v->value_len);
	if (!_t)	return(-1);
	v->vector = _t;
	if (v->flags & VECTOR_FLAG_INITZERO)
		memset(v->vector + (v->alloc_len * v->value_len), 0, (new_sz + 2) - v->alloc_len);
	v->alloc_len = new_sz;
	return(0);
}


int
vector_set(vector *v, uint32_t index, void *value)
{
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_lock(&v->LOCK);
	if (index >= v->alloc_len)
		if (0 != vector_resize(v, index+1))	return(-1);
	memcpy(v->vector + (index * v->value_len), value, v->value_len);
	if (index > v->len)	v->len = index;
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_unlock(&v->LOCK);
	return(0);
}

int
vector_append(vector *v, void *value)
{
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_lock(&v->LOCK);
	if (v->len + 1 >= v->alloc_len)
		if (0 != vector_resize(v, v->len + 2))	return(-1);
	memcpy(v->vector + ((v->len + 1) * v->value_len), value, v->value_len);
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_unlock(&v->LOCK);
	return(0);
	
}

// Copy the vector element into the pointer I give

int
vector_get(vector *v, uint32_t index, void *value_p)
{
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_lock(&v->LOCK);
	if (index >= v->alloc_len)
		return(-1);
	memcpy(value_p, v->vector + (index * v->value_len), v->value_len);
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_unlock(&v->LOCK);
	return(0);
}

void *
vector_getp(vector *v, uint32_t index)
{
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_lock(&v->LOCK);
	if (index >= v->alloc_len)
		return(0);
	void *r = v->vector + (index * v->value_len);
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		pthread_mutex_unlock(&v->LOCK);
	return( r );
}

void *
vector_getp_vlock(vector *v, uint32_t index, pthread_mutex_t **vlock)
{
	if (!v->flags & VECTOR_FLAG_BIGLOCK)
		return(0);
	if (index >= v->alloc_len)
		return(0);
	pthread_mutex_lock(&v->LOCK);
	*vlock = &v->LOCK;
	return(v->vector + (index * v->value_len));
}	


