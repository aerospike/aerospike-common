#ifndef _AS_STRING_H
#define _AS_STRING_H

#include "as_val.h"

typedef struct as_string_s as_string;

int as_string_free(as_string *);

as_string * as_string_new(const char *);

const char * as_string_tostring(const as_string *);

as_val * as_string_toval(const as_string *);

as_string * as_string_fromval(const as_val *);

#endif // _AS_STRING_H