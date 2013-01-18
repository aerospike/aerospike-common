#include "as_string.h"
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline as_string *   as_string_new(char *);
extern inline as_string *   as_string_init(as_string *, char *);
extern inline int           as_string_destroy(as_string *);
extern inline int           as_string_free(as_string *);

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
