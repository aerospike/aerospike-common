#pragma once

#include <sys/types.h>
#include <string.h>

#include "as_util.h"
#include "as_val.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_string_s as_string;

struct as_string_s {
    as_val _;
    bool value_is_malloc; 
    char * value;
    size_t len;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_val as_string_val;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_string *    as_string_init(as_string *, char *s, bool is_malloc);
as_string *   as_string_new(char *, bool is_malloc);

void           as_string_destroy(as_string *);
void    		as_string_val_destroy(as_val *v);

uint32_t as_string_val_hash(const as_val *);
char *as_string_val_tostring(const as_val *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline char * as_string_tostring(const as_string * s) {
    return (s->value);
}

size_t as_string_len(as_string * s);

uint32_t as_string_hash(const as_string * s);

inline as_val * as_string_toval(const as_string * s) {
    return (as_val *)s;
}

inline as_string * as_string_fromval(const as_val * v) {
    return as_util_fromval(v, AS_STRING, as_string);
}
