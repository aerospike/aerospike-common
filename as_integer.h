#pragma once

#include "as_util.h"
#include "as_val.h"
#include <inttypes.h>

/******************************************************************************
 *
 * TYPE DECLARATIONS
 * 
 ******************************************************************************/

typedef struct as_integer_s as_integer;

/******************************************************************************
 *
 * TYPE DEFINITIONS
 * 
 ******************************************************************************/

struct as_integer_s {
    as_val _;
    int64_t value;
};

/******************************************************************************
 *
 * FUNCTION DECLARATIONS
 * 
 ******************************************************************************/

as_integer * as_integer_new(int64_t);

int as_integer_init(as_integer *, int64_t);

int as_integer_free(as_integer *);

int as_integer_inc(as_integer *);

int64_t as_integer_toint(const as_integer *);

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS – CONVERSIONS
 * 
 ******************************************************************************/

inline as_val * as_integer_toval(const as_integer * i) {
    return (as_val *)i;
}

inline as_integer * as_integer_fromval(const as_val * v) {
    return as_util_fromval(v, AS_INTEGER, as_integer);
}