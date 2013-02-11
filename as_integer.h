#pragma once

#include <inttypes.h>

#include "as_util.h"
#include "as_val.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_integer_s as_integer;

struct as_integer_s {
    as_val _;
    int64_t value;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_integer *  as_integer_init(as_integer *val, int64_t i);
as_integer *  as_integer_new(int64_t i);

void 		 as_integer_val_destroy(as_val *v);

uint32_t	 as_integer_val_hash(const as_val *v);
char 		*as_integer_val_tostring(const as_val *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline void as_integer_destroy(as_integer *i) {
	return;
}

inline int64_t as_integer_toint(const as_integer * i) {
    return i->value;
}

inline uint32_t as_integer_hash(const as_integer * i) {
    return (uint32_t) i->value;
}


inline as_val * as_integer_toval(const as_integer * i) {
    return (as_val *) i;
}

inline as_integer * as_integer_fromval(const as_val * v) {
    return as_util_fromval(v, AS_INTEGER, as_integer);
}