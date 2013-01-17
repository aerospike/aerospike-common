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
 * FUNCTIONS
 ******************************************************************************/

int as_string_init(as_string *, char *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_string_destroy(as_string * s) {
    if ( s->value ) {
        free(s->value);
        s->value = NULL;
    }
    s->len = 0;
    return 0;
}

inline as_string * as_string_new(char * s) {
    as_string * v = (as_string *) malloc(sizeof(as_string));
    as_string_init(v, s);
    return v;
}

inline int as_string_free(as_string * s) {
    as_string_destroy(s);
    free(s);
    return 0;
}

inline char * as_string_tostring(const as_string * s) {
    return (s ? s->value : NULL);
}

inline size_t as_string_len(as_string * s) {
    return ( s && s->value ? ( s->len ? s->len : ( s->len = strlen(s->value) ) ) : 0 );
}

inline as_val * as_string_toval(const as_string * s) {
    return (as_val *)s;
}

inline as_string * as_string_fromval(const as_val * v) {
    return as_util_fromval(v, AS_STRING, as_string);
}
