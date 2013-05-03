#pragma once

#include "as_util.h"
#include "as_val.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_nil_s as_nil;

struct as_nil_s {
    as_val _;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

//

as_nil * as_nil_init(as_nil * v);
as_nil * as_nil_new();

void as_nil_destroy(as_nil * v);
void as_nil_val_destroy(as_val * v);

uint32_t as_nil_val_hash(const as_val *v);
char *as_nil_val_tostring(const as_val *v);

//
// Exposed implementation calls
//

uint32_t as_nil_hash(const as_nil *n);

inline as_val * as_nil_toval(as_nil * n) {
	return( (as_val *) n );
}

inline as_nil * as_nil_fromval(as_val * v) {
	return( (as_nil *) v );
}
