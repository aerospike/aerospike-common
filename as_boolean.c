#include "as_boolean.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_boolean *  as_boolean_new(bool);
extern inline int           as_boolean_free(as_boolean *);

extern inline as_boolean *  as_boolean_init(as_boolean *, bool);
extern inline int           as_boolean_destroy(as_boolean *);

extern inline bool          as_boolean_tobool(const as_boolean *);

extern inline uint32_t      as_boolean_hash(const as_boolean *);
extern inline as_val *      as_boolean_toval(const as_boolean *);
extern inline as_boolean *  as_boolean_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int      as_boolean_val_free(as_val *);
static uint32_t as_boolean_val_hash(as_val *);
static char *   as_boolean_val_tostring(as_val *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_val as_boolean_val = {
    .type       = AS_BOOLEAN, 
    .size       = sizeof(as_boolean),
    .free       = as_boolean_val_free, 
    .hash       = as_boolean_val_hash,
    .tostring   = as_boolean_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/


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
