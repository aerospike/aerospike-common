/*
 * A general purpose vector
 * Uses locks, so only moderately fast
 * If you need to deal with sparse data, really sparse data,
 * use a hash table. This assumes that packed data is a good idea.
 * Does the fairly trivial realloc thing for extension,
 * so 
 * And you can keep adding cool things to it
 * Copywrite 2008 Brian Bulkowski
 * All rights reserved
 */

#pragma once
 
#include <inttypes.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#define CITRUSLEAF 1



#ifdef CITRUSLEAF
#include "cf.h"
#else
typedef uint8_t byte;
#define cf_detail( __UNIT, __fmt, __args...) fprintf(stderr, "DETAIL"__fmt, ## __args)
#define cf_debug( __UNIT, __fmt, __args...) fprintf(stderr, "DEBUG"__fmt, ## __args)
#define cf_info( __UNIT, __fmt, __args...) fprintf(stderr, "INFO"__fmt, ## __args)
#endif



typedef struct vector_s {
	uint32_t value_len;
	uint flags;
	uint alloc_len; // number of elements currently allocated
	uint len;       // number of elements in table, largest element set
	uint8_t *vector;
	bool	stack_struct;
	bool	stack_vector;
	pthread_mutex_t		LOCK;
} vector;


#define VECTOR_ELEM_SZ(_v) ( _h->value_len )

#define VECTOR_FLAG_BIGLOCK 0x01 // support multithreaded access with a single big lock
#define VECTOR_FLAG_INITZERO 0x02 // internally init the vector objects to 0
#define VECTOR_FLAG_BIGRESIZE 0x04 // appends will be common - speculatively allocate extra memory
#define VECTOR_REDUCE_DELETE (1) // indicate that a delete should be done during the reduction


/*
 * Create a vector with malloc for handing around
 */

vector *
vector_create(uint32_t value_len, uint32_t init_sz, uint flags);

/*
** create a stack vector, but with an allocated internal-vector-bit
*/

int
vector_init(vector *v, uint32_t value_len, uint32_t init_sz, uint flags);

void
vector_init_smalloc(vector *v, uint32_t value_len, uint8_t *sbuf, int sbuf_sz, uint flags);

#define vector_define(__x, __value_len, __flags) \
	uint8_t vector##__x[1024]; vector __x; vector_init_smalloc(&__x, __value_len, vector##__x, sizeof(vector##__x), __flags);

/*
** todo: static allocate a vector, with passed-in memory?
*/


/* Place a value into the hash
 * Value will be copied into the hash
 */
extern int vector_set(vector *v, uint32_t index, void *value);
extern int vector_get(vector *v, uint32_t index, void *value);
// this is very dangerous if it's a multithreaded vector. Use _vlock if multithrad.
extern void * vector_getp(vector *v, uint32_t index);
extern void * vector_getp_vlock(vector *v, uint32_t index, pthread_mutex_t **vlock);
extern int vector_append(vector *v, void *value);

#define vector_reset( __v ) (__v)->len = 0; if ( (__v)->flags & VECTOR_FLAG_INITZERO) memset( (__v)->vector, 0, (__v)->alloc_len * (__v)->value_len);

// Adds a an element to the end, only if it doesn't exist already
// uses a bit-by-bit compare, thus is O(N) against the current length
// of the vector
extern int vector_append_unique(vector *v, void *value);

// Deletes an element by moving all the remaining elements down by one
extern int vector_delete(vector *v, uint32_t index);
// Delete a range in the vector. Inclusive. Thus:
//   a vector with len 5, you could delete start=0, end=3, leaving one element at the beginning (slot 0)
//   don't set start and end the same, that's a single element delete, use vector_delete instead
//   (or change the code to support that!)
//   returns -1 on bad ranges
extern int vector_delete_range(vector *v, uint32_t start_index, uint32_t end_index);

// There may be more allocated than you need. Fix that.
//
extern void vector_compact(vector *v);


/*
** Get the number of elements currently in the vector
*/
static inline uint32_t vector_size(vector *v)
{
	return(v->len);	
}


/*
 * Destroy the entire hash - all memory will be freed
 */
extern void vector_destroy(vector *v);

/*
** nice wrapper functions
** very common vector types are pointers, and integers
*/

static inline vector *vector_pointer_create(uint32_t init_sz, uint32_t flags)
{
	return(vector_create(sizeof(void *), init_sz, flags));
}

static inline int vector_pointer_init(vector *v, uint32_t init_sz, uint32_t flags)
{
	return(vector_init(v, sizeof(void *), init_sz, flags));
}

static inline int vector_pointer_set(vector *v, uint32_t index, void *p)
{
	return(vector_set(v, index, &p));
}

static inline void * vector_pointer_get(vector *v, uint32_t index) {
	void *p;
	vector_get(v, index, &p);
	return(p);
}

static inline int vector_pointer_append(vector *v, void *p)
{
	return(vector_append(v, &p));
}

/*
** integer vectors!
*/

static inline vector *vector_integer_create(uint32_t init_sz, uint32_t flags)
{
	return(vector_create(sizeof(int), init_sz, flags));
}

static inline int vector_integer_init(vector *v, uint32_t init_sz, uint32_t flags)
{
	return(vector_init(v, sizeof(int), init_sz, flags));
}

static inline int vector_integer_set(vector *v, uint32_t index, int i)
{
	return(vector_set(v, index, &i));
}

static inline int vector_integer_get(vector *v, uint32_t index) {
	int i;
	vector_get(v, index, &i);
	return(i);
}

static inline int vector_integer_append(vector *v, int i)
{
	return(vector_append(v, &i));
}

