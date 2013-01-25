#pragma once

#include "as_val.h"
#include <stdbool.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_result_s as_result;

struct as_result_s {
    bool is_success;
    as_val * value;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_result * as_result_init(as_result *, bool, as_val *);
int as_result_destroy(as_result *);

as_result * as_success(as_val *);
as_result * as_failure(as_val *);
int as_result_free(as_result *);

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