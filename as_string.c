#include "as_string.h"
#include <stdlib.h>
#include <string.h>
#include <cf_alloc.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline char *        as_string_tostring(const as_string *);
extern inline size_t        as_string_len(as_string *);

extern inline uint32_t      as_string_hash(as_string *);
extern inline as_val *      as_string_toval(const as_string *);
extern inline as_string *   as_string_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int      as_string_val_free(as_val *);
static uint32_t as_string_val_hash(as_val *);
static char *   as_string_val_tostring(as_val *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_val as_string_val = {
    .type       = AS_STRING, 
    .size       = sizeof(as_string),
    .free       = as_string_val_free,
    .hash       = as_string_val_hash,
    .tostring   = as_string_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_string * as_string_init(as_string * v, char * s) {
    if ( !v ) return v;
    v->_ = as_string_val;
    v->value = s;
    v->len = 0;
    return v;
}

as_string * as_string_new(char * s) {
    as_string * v = (as_string *) cf_rc_alloc(sizeof(as_string));
    return as_string_init(v, s);
}

int as_string_destroy(as_string * s) {
    if ( !s ) return 0;
    if ( s->value ) free(s->value);
    s->value = NULL;
    s->len = 0;
    return 0;
}

int as_string_free(as_string * s) {
    if ( !s ) return 0;
    if ( cf_rc_release(s) > 0 ) return 0;
    as_string_destroy(s);
    cf_rc_free(s);
    return 0;
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

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
    size_t st = 3 + sl;
    char * str = (char *) malloc(sizeof(char) * st);
    *(str + 0) = '\"';
    strcpy(str + 1, s->value);
    *(str + 1 + sl) = '\"';
    *(str + 1 + sl + 1) = '\0';
    return str;
}
