#include "as_boolean.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const as_val AS_BOOLEAN_VAL;

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_val * as_boolean_toval(const as_boolean *);
extern inline as_boolean * as_boolean_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_boolean_val_free(as_val *);
static uint32_t as_boolean_val_hash(as_val *);
static char * as_boolean_val_tostring(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_val AS_BOOLEAN_VAL = {
    .type       = AS_BOOLEAN, 
    .size       = sizeof(as_boolean),
    .free       = as_boolean_val_free, 
    .hash       = as_boolean_val_hash,
    .tostring   = as_boolean_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_boolean * as_boolean_new(bool b) {
    as_boolean * v = (as_boolean *) malloc(sizeof(as_boolean));
    as_boolean_init(v,b);
    return v;
}

int as_boolean_init(as_boolean * v, bool b) {
    v->_ = AS_BOOLEAN_VAL;
    v->value = b;
    return 0;
}

int as_boolean_free(as_boolean * b) {
    free(b);
    return 0;
}

uint32_t as_boolean_hash(const as_boolean * b) {
    return b->value ? 1 : 0;
}

bool as_boolean_tobool(const as_boolean * b) {
    return b->value;
}

static int as_boolean_val_free(as_val * v) {
    return as_val_type(v) == AS_BOOLEAN ? as_boolean_free((as_boolean *) v) : 1;
}

static uint32_t as_boolean_val_hash(as_val * v) {
    return as_val_type(v) == AS_BOOLEAN ? as_boolean_hash((as_boolean *) v) : 0;
}

static char * as_boolean_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_BOOLEAN ) return NULL;

    as_boolean * b = (as_boolean *) v;
    char * str = (char *) malloc(sizeof(char) * 6);
    bzero(str,6);
    if ( b->value ) {
        strcpy(str,"true");
    }
    else {
        strcpy(str,"false");
    }
    return str;

}
