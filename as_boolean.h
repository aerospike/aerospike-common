#pragma once

#include "as_util.h"
#include "as_val.h"
#include <stdbool.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_boolean_s as_boolean;

struct as_boolean_s {
    as_val _;
    bool value;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_val as_boolean_val;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_boolean *  as_boolean_init(as_boolean *, bool);
int           as_boolean_destroy(as_boolean *);

as_boolean *  as_boolean_new(bool);
int           as_boolean_free(as_boolean *);

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
