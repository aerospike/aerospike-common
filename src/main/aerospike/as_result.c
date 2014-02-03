/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>

#include <citrusleaf/cf_alloc.h>
#include <aerospike/as_result.h>

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
    if (!r) return r;
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


