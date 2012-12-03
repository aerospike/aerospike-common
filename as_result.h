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
 * INLINE FUNCTIONS
 ******************************************************************************/


inline int as_result_init(as_result * r, bool is_success, as_val * v) {
    r->is_success = is_success;
    r->value = v;
    return 0;
}

inline int as_result_destroy(as_result * r) {
    as_result_init(r, false, NULL);
    return 0;
}

inline int as_result_free(as_result * r) {
    as_result_destroy(r);
    return 0;
}

inline as_result * as_success(as_val * v) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    as_result_init(r, true, v);
    return r;
}

inline as_result * as_failure(as_val * e) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    as_result_init(r, false, e);
    return r;
}

inline int as_result_tosuccess(as_result * r, as_val * v) {
    return as_result_init(r, true, v);
}

inline int as_result_tofailure(as_result * r, as_val * e) {
    return as_result_init(r, false, e);
}