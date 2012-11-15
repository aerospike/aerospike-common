#pragma once

#include "as_val.h"
#include <stdbool.h>

typedef struct as_boolean_s as_boolean;

int as_boolean_free(as_boolean *);

as_boolean * as_boolean_new(bool);

bool as_boolean_tobool(const as_boolean *);

as_val * as_boolean_toval(const as_boolean *);

as_boolean * as_boolean_fromval(const as_val *);
