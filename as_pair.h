#pragma once

#include "as_val.h"
#include <sys/types.h>

typedef struct as_pair_s as_pair;

struct as_pair_s {
    as_val _;
    as_val * _1;
    as_val * _2;
};



int as_pair_free(as_pair *);

as_pair * as_pair_new(as_val * _1, as_val * _2);

as_val * as_pair_1(as_pair * p);

as_val * as_pair_2(as_pair * p);

as_val * as_pair_toval(const as_pair *);

as_pair * as_pair_fromval(const as_val *);

#define pair(a,b) as_pair_new((as_val *) a, (as_val *) b)
