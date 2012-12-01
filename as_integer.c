#include "as_integer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_val * as_integer_toval(const as_integer *);
extern inline as_integer * as_integer_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_integer_val_free(as_val *);
static uint32_t as_integer_val_hash(as_val *);
static char * as_integer_val_tostring(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_val AS_INTEGER_VAL = {
    .type       = AS_INTEGER, 
    .size       = sizeof(as_integer),
    .free       = as_integer_val_free, 
    .hash       = as_integer_val_hash,
    .tostring   = as_integer_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_integer * as_integer_new(int64_t i) {
    as_integer * v = (as_integer *) malloc(sizeof(as_integer));
    as_integer_init(v, i);
    return v;
}

int as_integer_init(as_integer * v, int64_t i) {
    v->_ = AS_INTEGER_VAL;
    v->value = i;
    return 0;
}

int as_integer_free(as_integer * i) {
    free(i);
    return 0;
}

int as_integer_inc(as_integer * i) {
    i->value = i->value + 1;
    return 0;
}

int64_t as_integer_toint(const as_integer * i) {
    return i->value;
}

static int as_integer_val_free(as_val * v) {
    return as_val_type(v) == AS_INTEGER ? as_integer_free((as_integer *) v) : 1;
}

static uint32_t as_integer_val_hash(as_val * v) {
    return as_val_type(v) == AS_INTEGER ? (uint32_t) ((as_integer *) v)->value : 0;
}

static char * as_integer_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_INTEGER ) return NULL;
    as_integer * i = (as_integer *) v;
    char * str = (char *) malloc(sizeof(char) * 32);
    sprintf(str,"%ld",i->value);
    return str;
}
