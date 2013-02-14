#include <stdlib.h>
#include <cf_alloc.h>

#include "as_result.h"

#include "internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_result * as_result_tosuccess(as_result *, as_val *);
extern inline as_result * as_result_tofailure(as_result *, as_val *);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_result * as_result_init(as_result * r, bool is_success, as_val * v) {
    r->is_malloc = false;
    r->count = 1;
    r->is_success = is_success;
    r->value = v;
    return r;
}

as_result * as_result_new(bool is_success, as_val * v) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    r->is_malloc = true;
    r->count = 1;
    r->is_success = is_success;
    r->value = v;
    return r;
}

void as_result_destroy(as_result *r) {
    // if we reach the last reference, call the destructor, and free
    if ( 0 == cf_atomic32_decr(&(r->count)) ) {
        as_val_destroy(r->value); 
        if (r->is_malloc) {
            free(r);
        }
    }
    return;
}


as_result * as_success(as_val * v) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    return as_result_init(r, true, v);
}

as_result * as_failure(as_val * e) {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    return as_result_init(r, false, e);
}

as_result * as_result_reserve(as_result * r) {
    cf_atomic32_incr(&(r->count));
    return r;
}
