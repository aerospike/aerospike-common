#pragma once

#include "as_val.h"
#include <sys/types.h>

typedef struct as_pair_s as_pair;

int as_pair_free(as_pair *);

as_pair * as_pair_new(as_val * _1, as_val * _2);

as_val * as_string_toval(const as_pair *);

as_pair * as_string_fromval(const as_val *);
