#include <stdlib.h>
#include <cf_alloc.h>

#include "as_result.h"

#include "internal.h"

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_result * as_result_init(as_result * r) {
    r->is_malloc = false;
    r->count = 1;
    r->is_success = false;
    r->value = NULL;
    return r;
}

as_result * as_result_new() {
    as_result * r = (as_result *) malloc(sizeof(as_result));
    r->is_malloc = true;
    r->count = 1;
    r->is_success = false;
    r->value = NULL;
    return r;
}

as_result * as_result_reserve(as_result * r) {
    cf_atomic32_incr(&(r->count));
    return r;
}

void as_result_destroy(as_result *r) {
    // if we reach the last reference, call the destructor, and free
    if ( 0 == cf_atomic32_decr(&(r->count)) ) {
        if (r->value) {
            as_val_destroy(r->value); 
            r->value = NULL;
        }
        if (r->is_malloc) {
            free(r);
        }
    }
    return;
}


as_result * as_success_new(as_val * v) {
    as_result *r = as_result_new();
    r->is_success = true;
    r->value = v;
    return(r);
}

as_result * as_failure_new(as_val * v) {
    as_result *r = as_result_new();
    r->is_success = false;
    r->value = v;
    return(r);
}

// These functions init an as_result object - consume the as_val
as_result * as_success_init(as_result *r, as_val *v) {
    as_result_init(r);
    r->is_success = true;
    r->value = v;
    return(r);
}

as_result * as_failure_init(as_result *r, as_val *v) {
    as_result_init(r);
    r->is_success = false;
    r->value = v;
    return(r);
}

as_val * as_result_value(as_result *r) {
    return( r->value );
}


// These helper functions reset an initialized result
// to the following value
as_result * as_result_setsuccess(as_result * r, as_val * v) {
    if (r->value) as_val_destroy(r->value);
    r->value = v;
    r->is_success = true;
    return(r);
}

as_result * as_result_setfailure(as_result * r, as_val * v) {
    if (r->value) as_val_destroy(r->value);
    r->value = v;
    r->is_success = false;
    return(r);
}


