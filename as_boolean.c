#include "as_boolean.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cf_alloc.h>
#include "as_internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/
 
extern inline bool as_boolean_tobool(const as_boolean * b);
extern inline uint32_t as_boolean_hash(const as_boolean * b) ;
extern inline as_val * as_boolean_toval(const as_boolean * b);
extern inline as_boolean * as_boolean_fromval(const as_val * v);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_boolean * as_boolean_init(as_boolean * v, bool b) {
    as_val_init(&v->_, AS_BOOLEAN, false /*is_rcalloc*/);
    v->value = b;
    return v;
}

as_boolean * as_boolean_new(bool b) {
    as_boolean * v = (as_boolean *) malloc(sizeof(as_boolean));
    as_val_init(&v->_, AS_BOOLEAN, true /*is_rcalloc*/);
    v->value = b;
    return v;
}

void as_boolean_destroy(as_boolean * b) {
    return;
}

uint32_t as_boolean_val_hash(const as_val * v) {
    return as_boolean_hash((const as_boolean *)v);
}

char * as_boolean_val_tostring(const as_val * v) {
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
