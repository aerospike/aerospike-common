#include <string.h>
#include <cf_alloc.h>

#include "as_val.h"
#include "as_nil.h"

#include "internal.h"

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

as_nil * as_nil_init(as_nil * n) {
    as_val_init(&n->_, AS_NIL, false/*is_malloc*/);
    return n;
}

as_nil * as_nil_new() {
    as_nil * n = (as_nil *) malloc(sizeof(as_nil));
    as_val_init(&n->_, AS_NIL, true/*is_malloc*/);
    return n;
}

void as_nil_destroy(as_nil * v) {
    return;
}

void as_nil_val_destroy(as_val * v) {
    return;
}

uint32_t as_nil_hash(const as_nil * v) {
	return(0);
}

char *as_nil_val_tostring(const as_val *v) {
	return(strdup("NIL"));
}
