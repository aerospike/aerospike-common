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

#include <string.h>
#include <stdlib.h>

#include <citrusleaf/cf_vector.h>

#include <citrusleaf/alloc.h>

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define VECTOR_LOCK(_v) 	pthread_mutex_lock(&_v->LOCK)
#define VECTOR_UNLOCK(_v) 	pthread_mutex_unlock(&_v->LOCK)

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cf_vector * cf_vector_create( uint32_t value_len, uint32_t init_sz, uint flags) {
	cf_vector *v;

	v = cf_malloc(sizeof(cf_vector));
	if (!v)	return(0);

	v->value_len = value_len;
	v->flags = flags;
	v->alloc_len = init_sz;
	v->len = 0;
	v->stack_struct = false;
	v->stack_vector = false;
	if (init_sz) {
		v->vector = cf_malloc(init_sz * value_len);
		if (!v->vector)	{
			cf_free(v);
			return(0);
		}
	}
	else
		v->vector = 0;
	if ((flags & VECTOR_FLAG_INITZERO) && v->vector)
		memset(v->vector, 0, init_sz * value_len);
	if (flags & VECTOR_FLAG_BIGLOCK){
		pthread_mutex_init(&v->LOCK, 0);
	}
	return(v);
}

int cf_vector_init(cf_vector *v, uint32_t value_len, uint32_t init_sz, uint flags) {
	v->value_len = value_len;
	v->flags = flags;
	v->alloc_len = init_sz;
	v->len = 0;
	v->stack_struct = true;
	v->stack_vector = false;
	if (init_sz) {
		v->vector = cf_malloc(init_sz * value_len);
		if (!v->vector)	return(-1);
	}
	else
		v->vector = 0;
	if ((flags & VECTOR_FLAG_INITZERO) && v->vector)
		memset(v->vector, 0, init_sz * value_len);
	if (flags & VECTOR_FLAG_BIGLOCK){
		pthread_mutex_init(&v->LOCK, 0);
	}
	return(0);
}

void cf_vector_init_smalloc(cf_vector *v, uint32_t value_len, uint8_t *sbuf, int sbuf_sz, uint flags) {
	v->value_len = value_len;
	v->flags = flags;
	v->alloc_len = sbuf_sz / value_len;
	v->len = 0;
	v->stack_struct = true;
	v->stack_vector = true;
	v->vector = sbuf;
	if ((flags & VECTOR_FLAG_INITZERO) && v->vector)
		memset(v->vector, 0, sbuf_sz);
	if (flags & VECTOR_FLAG_BIGLOCK){
		pthread_mutex_init(&v->LOCK, 0);
	}
}

void cf_vector_clone_stack(cf_vector *v, cf_vector *target, uint8_t *target_buf) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);

	target->value_len = v->value_len;
	target->flags = v->flags;
	target->alloc_len = v->len;
	target->len = v->len;
	target->stack_struct = true;
	target->stack_vector = true;
	target->vector = target_buf;
	memcpy(target->vector, v->vector, v->value_len * v->len);

	if (target->flags & VECTOR_FLAG_BIGLOCK){
		pthread_mutex_init(&target->LOCK, 0);
	}

	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
}

void cf_vector_destroy(cf_vector *v) {
	if (v->flags & VECTOR_FLAG_BIGLOCK){
		pthread_mutex_destroy(&v->LOCK);
	}
	if (v->vector && (v->stack_vector == false))	cf_free(v->vector);
	if (v->stack_struct == false) cf_free(v);
}

static int cf_vector_resize(cf_vector *v, uint32_t new_sz) {
	if (v->flags & VECTOR_FLAG_BIGRESIZE) {
		if (new_sz < 50)	new_sz = 50;
	}
	else if (new_sz == 0) {
		new_sz = 2;
	}
	uint8_t *_t;
	if (v->vector == 0 || v->stack_vector) {
		_t = cf_malloc(new_sz * v->value_len);
		if (!_t)	return(-1);
		if (v->stack_vector) {
			memcpy(_t, v->vector, v->alloc_len * v->value_len); 
			v->stack_vector = false;
		}
	}
	else
		_t = cf_realloc(v->vector, (new_sz) * v->value_len);
	if (!_t)	return(-1);
	v->vector = _t;
	if (v->flags & VECTOR_FLAG_INITZERO)
		memset(v->vector + (v->alloc_len * v->value_len), 0, (new_sz - v->alloc_len) * v->value_len);
	v->alloc_len = new_sz;
	return(0);
}

int cf_vector_set(cf_vector *v, uint32_t index, void *value) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);

	// Return error if index is not within current array bounds.
	if (index >= v->alloc_len)
		return(-1);

	memcpy(v->vector + (index * v->value_len), value, v->value_len);
	if (index >= v->len)	v->len = index + 1;
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return(0);
}

int cf_vector_append_lockfree(cf_vector *v, void *value) {
	if (v->len >= v->alloc_len)
		if (0 != cf_vector_resize(v, v->len * 2))	return(-1);
	memcpy(v->vector + (v->len * v->value_len), value, v->value_len);
	v->len ++;
	return(0);
	
}



int cf_vector_append(cf_vector *v, void *value) {
	int rv;
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	rv = cf_vector_append_lockfree(v, value);
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return(rv);
}

int cf_vector_append_unique(cf_vector *v, void *value) {
	int rv=0;
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	uint8_t	*_b = v->vector;
	uint32_t	_l = v->value_len;
	for (uint i=0;i<v->len;i++) {
		if (0 == memcmp(value, _b, _l)) {
			goto Found;
		}
		_b += _l;
	}
	rv = cf_vector_append_lockfree(v, value);
Found:	
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return(rv);
}

// Copy the vector element into the pointer I give

int cf_vector_get(cf_vector *v, uint32_t index, void *value_p) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	if (index >= v->alloc_len)
		return(-1);
	memcpy(value_p, v->vector + (index * v->value_len), v->value_len);
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return(0);
}

void * cf_vector_getp(cf_vector *v, uint32_t index) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	if (index >= v->alloc_len)
		return(0);
	void *r = v->vector + (index * v->value_len);
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return( r );
}

void * cf_vector_getp_vlock(cf_vector *v, uint32_t index, pthread_mutex_t **vlock) {
	if (!v->flags & VECTOR_FLAG_BIGLOCK)
		return(0);
	if (index >= v->alloc_len)
		return(0);
	VECTOR_LOCK(v);
	*vlock = &v->LOCK;
	return(v->vector + (index * v->value_len));
}

int cf_vector_delete(cf_vector *v, uint32_t index) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	// check bounds
	if (index >= v->len)
		return (-1);
	// check for last - no copy
	if (index != v->len - 1) {
		memmove(v->vector + (index * v->value_len), 
				v->vector + ((index+1) * v->value_len),
				(v->len - (index+1)) * v->value_len );
	}
	v->len --;
	
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return(0);
}

int cf_vector_delete_range(cf_vector *v, uint32_t idx_start, uint32_t idx_end) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	// check bounds
	if (idx_start >= idx_end)
		return (-1);
	if (idx_start >= v->len)
		return(-1);
	if (idx_end >= v->len)
		return(-1);
	
	// Copy down if not at end
	if (idx_end != v->len - 1) {
		memmove( v->vector + (idx_start * v->value_len),
				v->vector + ((idx_end+1) * v->value_len),
			    (v->len - (idx_end+1)) * v->value_len );
	
	}
	v->len -= (idx_end - idx_start) + 1;
	
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return(0);
}

void cf_vector_compact(cf_vector *v) {
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_LOCK(v);
	if (v->alloc_len && (v->len != v->alloc_len)) {
		v->vector = cf_realloc(v->vector, v->len * v->alloc_len);
		v->alloc_len = v->len;
	}
	if (v->flags & VECTOR_FLAG_BIGLOCK)
		VECTOR_UNLOCK(v);
	return;
}
