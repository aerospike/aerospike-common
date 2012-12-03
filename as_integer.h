#pragma once

#include "as_util.h"
#include "as_val.h"
#include <inttypes.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_integer_s as_integer;

struct as_integer_s {
    as_val _;
    int64_t value;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_integer_init(as_integer *, int64_t);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_integer_destroy(as_integer * i) {
    i->value = 0;
    return 0;
}

inline as_integer * as_integer_new(int64_t i) {
    as_integer * v = (as_integer *) malloc(sizeof(as_integer));
    as_integer_init(v, i);
    return v;
}

inline int as_integer_free(as_integer * i) {
    free(i);
    return 0;
}

inline int64_t as_integer_toint(const as_integer * i) {
    return i->value;
}

inline as_val * as_integer_toval(const as_integer * i) {
    return (as_val *)i;
}

inline as_integer * as_integer_fromval(const as_val * v) {
    return as_util_fromval(v, AS_INTEGER, as_integer);
}