#pragma once

#include "as_util.h"
#include "as_val.h"
#include <sys/types.h>

/******************************************************************************
 *
 * TYPE DECLARATIONS
 * 
 ******************************************************************************/

typedef struct as_string_s as_string;

/******************************************************************************
 *
 * TYPE DEFINITIONS
 * 
 ******************************************************************************/

struct as_string_s {
    as_val _;
    char * value;
    size_t len;
};

/******************************************************************************
 *
 * FUNCTION DECLARATIONS
 * 
 ******************************************************************************/

as_string * as_string_new(char *);

int as_string_init(as_string *, char *);

int as_string_free(as_string *);

char * as_string_tostring(const as_string *);

size_t as_string_len(as_string *);

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS - CONVERSIONS
 * 
 ******************************************************************************/

inline as_val * as_string_toval(const as_string * s) {
    return (as_val *)s;
}

inline as_string * as_string_fromval(const as_val * v) {
    return as_util_fromval(v, AS_STRING, as_string);
}
