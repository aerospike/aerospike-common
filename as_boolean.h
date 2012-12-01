#pragma once

#include "as_util.h"
#include "as_val.h"
#include <stdbool.h>

/******************************************************************************
 *
 * TYPE DECLARATIONS
 * 
 ******************************************************************************/

typedef struct as_boolean_s as_boolean;

/******************************************************************************
 *
 * TYPE DEFINITIONS
 * 
 ******************************************************************************/

struct as_boolean_s {
    as_val _;
    bool value;
};

/******************************************************************************
 *
 * GLOBAL FUNCTION DECLARATIONS
 * 
 ******************************************************************************/

int as_boolean_free(as_boolean *);

as_boolean * as_boolean_new(bool);

int as_boolean_init(as_boolean *, bool);

bool as_boolean_tobool(const as_boolean *);

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS 
 * 
 ******************************************************************************/

inline as_val * as_boolean_toval(const as_boolean * b) {
    return (as_val *)b;
}

inline as_boolean * as_boolean_fromval(const as_val * v) {
    return as_util_fromval(v, AS_BOOLEAN, as_boolean);
}
