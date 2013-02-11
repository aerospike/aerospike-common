#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cf_alloc.h>

#include "as_internal.h"

#include "as_integer.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void as_integer_destroy(as_integer *i) ;
extern inline int64_t as_integer_toint(const as_integer * i);
extern inline uint32_t as_integer_hash(const as_integer * i);
extern inline as_val * as_integer_toval(const as_integer * i) ;\
extern inline as_integer * as_integer_fromval(const as_val * v) ;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_integer * as_integer_init(as_integer * v, int64_t i) {
	as_val_init(&v->_, AS_INTEGER, false);
    v->value = i;
    return v;
}

as_integer * as_integer_new(int64_t i) {
    as_integer * v = (as_integer *) malloc(sizeof(as_integer));
    as_val_init(&v->_, AS_INTEGER, true /*is_malloc*/);
    v->value = i;
    return v;
}

void as_integer_val_destroy(as_val * v) {
    return;
}

uint32_t as_integer_val_hash(const as_val * v) {
    return as_integer_hash((as_integer *) v);
}

char * as_integer_val_tostring(const as_val * v) {
    as_integer * i = (as_integer *) v;
    char * str = (char *) malloc(sizeof(char) * 32);
    sprintf(str,"%ld",i->value);
    return str;
}
