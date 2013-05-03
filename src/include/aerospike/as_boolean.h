#pragma once

#include <stdbool.h>

#include "as_util.h"
#include "as_val.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_boolean_s as_boolean;

struct as_boolean_s {
    as_val _;
    bool value;
};


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_boolean *  as_boolean_init(as_boolean *, bool);
as_boolean *  as_boolean_new(bool);

void          as_boolean_destroy(as_boolean *);
void          as_boolean_val_destroy(as_val *);

uint32_t as_boolean_val_hash(const as_val * v);
char *as_boolean_val_tostring(const as_val *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/
 
inline bool as_boolean_tobool(const as_boolean * b) {
    return b->value;
}

inline uint32_t as_boolean_hash(const as_boolean * b) {
    return b->value ? 1 : 0;
}


inline as_val * as_boolean_toval(const as_boolean * b) {
    return (as_val *) b;
}

inline as_boolean * as_boolean_fromval(const as_val * v) {
    return as_util_fromval(v, AS_BOOLEAN, as_boolean);
}
