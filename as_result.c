#include "as_result.h"
#include <stdlib.h>
#include <cf_alloc.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_result * as_result_tosuccess(as_result *, as_val *);
extern inline as_result * as_result_tofailure(as_result *, as_val *);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_result * as_result_init(as_result * r, bool is_success, as_val * v) {
    if ( !r ) return r;
    r->is_success = is_success;
    r->value = as_val_ref(v);
    return r;
}

int as_result_destroy(as_result * r) {
    if ( !r ) return 0;
    r->is_success = false;
    if ( r->value ) as_val_free(r->value);
    r->value = NULL;
    return 0;
}


as_result * as_success(as_val * v) {
    as_result * r = (as_result *) cf_rc_alloc(sizeof(as_result));
    return as_result_init(r, true, v);
}

as_result * as_failure(as_val * e) {
    as_result * r = (as_result *) cf_rc_alloc(sizeof(as_result));
    return as_result_init(r, false, e);
}

int as_result_free(as_result * r) {
    if ( !r ) return 0;
    if ( cf_rc_release(r) > 0 ) return 0;
    as_result_destroy(r);
    cf_rc_free(r);
    return 0;
}


as_val * as_result_value(as_result * r) {
    return as_val_ref(r->value);
}