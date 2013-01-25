#pragma once

#include "as_util.h"
#include "as_val.h"
#include <sys/types.h>
#include <string.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_string_s as_string;

struct as_string_s {
    as_val _;
    char * value;
    size_t len;
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_val as_string_val;

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

as_string *   as_string_init(as_string *, char *);
int           as_string_destroy(as_string *);

as_string *   as_string_new(char *);
int           as_string_free(as_string *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline char * as_string_tostring(const as_string * s) {
    return (s ? s->value : NULL);
}

inline size_t as_string_len(as_string * s) {
    return ( s && s->value ? ( s->len ? s->len : ( s->len = strlen(s->value) ) ) : 0 );
}



inline uint32_t as_string_hash(as_string * s) {
    uint32_t hash = 0;
    int c;
    char * str = s->value;
    while ( (c = *str++) ) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

inline as_val * as_string_toval(const as_string * s) {
    return (as_val *)s;
}

inline as_string * as_string_fromval(const as_val * v) {
    return as_util_fromval(v, AS_STRING, as_string);
}
