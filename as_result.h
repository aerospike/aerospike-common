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

as_result * as_result_init(as_result *r);
as_result * as_result_new();

as_result * as_result_reserve(as_result *r);

void as_result_destroy(as_result *);

// These functions new an as_result object - consume the as_val
as_result * as_success_new(as_val *);
as_result * as_failure_new(as_val *);

// These functions init an as_result object - consume the as_val
as_result * as_success_init(as_result *, as_val *);
as_result * as_failure_init(as_result *, as_val *);

// retrieves the value associated with success or failure
as_val * as_result_value(as_result *);

as_result * as_result_setsuccess(as_result * r, as_val * v);
as_result * as_result_setfailure(as_result * r, as_val * v);
