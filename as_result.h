#ifndef _AS_RESULT_H
#define _AS_RESULT_H

#include "as_val.h"
#include <stdbool.h>

typedef struct as_result_s as_result;

struct as_result_s {
    bool is_success;
    as_val * value;
};

as_result * as_result_new();

as_result * as_success(as_val *);

as_result * as_failure(as_val *);

int as_result_tosuccess(as_result *, as_val *);

int as_result_tofailure(as_result *, as_val *);

#endif // _AS_RESULT_H