#pragma once

#include "as_val.h"
#include <inttypes.h>

typedef struct as_integer_s as_integer;

int as_integer_free(as_integer *);

as_integer * as_integer_new(int64_t);

int as_integer_inc(as_integer *);

int64_t as_integer_toint(const as_integer *);

as_val * as_integer_toval(const as_integer *);

as_integer * as_integer_fromval(const as_val *);