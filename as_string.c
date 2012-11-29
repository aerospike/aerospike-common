#include "as_string.h"
#include <stdlib.h>
#include <string.h>

struct as_string_s {
    as_val _;
    char * value;
    size_t len;
};


static const as_val AS_STRING_VAL;
static int as_string_freeval(as_val *);


/**
 * The `char *` passed as a parameter will be managed by the as_string going forward.
 * If you are going to continue to use the `char *`, then you probably should copy it first.
 */
as_string * as_string_new(char * s) {
    as_string * v = (as_string *) malloc(sizeof(as_string));
    v->_ = AS_STRING_VAL;
    v->value = s;
    v->len = 0;
    return v;
}

int as_string_free(as_string * s) {
    if ( !s ) return 0;
    if ( s->value ) free(s->value);
    free(s);
    return 0;
}

static uint32_t as_string_hash(as_string * s) {
    uint32_t hash = 0;
    int c;
    char * str = s->value;
    while (c = *str++) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return 0;
}

char * as_string_tostring(const as_string * s) {
    if ( !s ) return NULL;
    return s->value;
}

size_t as_string_len(as_string * s) {
    if ( !s ) return 0;
    if ( !s->value ) return 0;
    if ( !s->len ) return (s->len = strlen(s->value));
    return s->len;
}

as_val * as_string_toval(const as_string * s) {
    return (as_val *) s;
}

as_string * as_string_fromval(const as_val * v) {
    return as_val_type(v) == AS_STRING ? (as_string *) v : NULL;
}

static int as_string_freeval(as_val * v) {
    return as_val_type(v) == AS_STRING ? as_string_free((as_string *) v) : 1;
}

static uint32_t as_string_hashval(as_val * v) {
    return as_val_type(v) == AS_STRING ? as_string_hash((as_string *) v) : 0;
}

static const as_val AS_STRING_VAL = {
    .type   = AS_STRING, 
    .free   = as_string_freeval,
    .hash   = as_string_hashval
};