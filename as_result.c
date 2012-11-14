#include "as_result.h"
#include <stdlib.h>

as_result * as_success(as_val * value) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    r->is_success = true;
    r->value = value;
    return r;
}

as_result * as_failure(as_val * error) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    r->is_success = false;
    r->value = error;
    return r;
}

int as_result_tosuccess(as_result * r, as_val * value) {
    r->is_success = true;
    r->value = value;
    return 0;
}

int as_result_tofailure(as_result * r, as_val * error) {
    r->is_success = false;
    r->value = error;
    return 0;
}