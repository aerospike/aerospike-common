#pragma once

#include <stdbool.h>

#include "as_val.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_result_s as_result;

struct as_result_s {
    bool        is_malloc;
    cf_atomic32 count;
    bool is_success;
    as_val * value;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_result * as_result_init(as_result *, bool, as_val *);
int as_result_reserve(as_result *r);
void as_result_destroy(as_result *);

// These functions new an as_result object
as_result * as_success(as_val *);
as_result * as_failure(as_val *);

// retrieves the value associated with success or failure
as_val * as_result_value(as_result *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline as_result * as_result_tosuccess(as_result * r, as_val * v) {
    return as_result_init(r, true, v);
}

inline as_result * as_result_tofailure(as_result * r, as_val * e) {
    return as_result_init(r, false, e);
}