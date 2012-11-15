#pragma once

#include "as_val.h"
#include <sys/types.h>

typedef struct as_string_s as_string;

int as_string_free(as_string *);

as_string * as_string_new(char *);

char * as_string_tostring(const as_string *);

size_t as_string_len(as_string *);

as_val * as_string_toval(const as_string *);

as_string * as_string_fromval(const as_val *);
