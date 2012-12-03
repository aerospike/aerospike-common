#include "as_string.h"
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_string_destroy(as_string *);

extern inline as_string * as_string_new(char *);
extern inline int as_string_free(as_string *);

extern inline char * as_string_tostring(const as_string *);
extern inline size_t as_string_len(as_string *);

extern inline as_val * as_string_toval(const as_string *);
extern inline as_string * as_string_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_string_val_free(as_val *);
static uint32_t as_string_val_hash(as_val *);
static char * as_string_val_tostring(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_val AS_STRING_VAL = {
    .type       = AS_STRING, 
    .size       = sizeof(as_string),
    .free       = as_string_val_free,
    .hash       = as_string_val_hash,
    .tostring   = as_string_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_string_init(as_string * v, char * s) {
    v->_ = AS_STRING_VAL;
    v->value = s;
    v->len = 0;
    return 0;
}

static uint32_t as_string_hash(as_string * s) {
    uint32_t hash = 0;
    int c;
    char * str = s->value;
    while ( (c = *str++) ) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

static int as_string_val_free(as_val * v) {
    return as_val_type(v) == AS_STRING ? as_string_free((as_string *) v) : 1;
}

static uint32_t as_string_val_hash(as_val * v) {
    return as_val_type(v) == AS_STRING ? as_string_hash((as_string *) v) : 0;
}

static char * as_string_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_STRING ) return NULL;
    as_string * s = (as_string *) v;
    size_t sl = as_string_len(s);
    size_t st = 2 + sl;
    char * str = (char *) malloc(sizeof(char) * st);
    bzero(str,st);
    strcpy(str, "\"");
    strcpy(str+1, s->value);
    strcpy(str+1+sl, "\"");
    return str;
}