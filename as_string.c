#include "as_string.h"
#include <stdlib.h>
#include <string.h>
#include <cf_alloc.h>
#include "as_internal.h"

extern inline char * as_string_tostring(const as_string * s);
extern inline as_val * as_string_toval(const as_string * s);
extern inline as_string * as_string_fromval(const as_val * v);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_string * as_string_init(as_string * v, char * s, bool is_malloc) {
    as_val_init(&v->_, AS_STRING, false /*is_malloc*/);
    v->value_is_malloc = is_malloc;
    v->value = s;
    v->len = SIZE_MAX;
    return v;
}

as_string * as_string_new(char * s, bool is_malloc) {
    as_string * v = (as_string *) malloc(sizeof(as_string));
    as_val_init(&v->_, AS_STRING, true /*is_malloc*/);
    v->value_is_malloc = is_malloc;
    v->value = s;
    v->len = SIZE_MAX;
    return v;
}

void as_string_destroy(as_string * s) {
    if ( s->value_is_malloc && s->value ) free(s->value);
}

void as_string_val_destroy(as_val * v) {
   as_string_destroy( (as_string *)v );
}

size_t as_string_len(as_string * s) {
	if (s->len == SIZE_MAX) s->len = strlen(s->value);
	return(s->len);
}

uint32_t as_string_hash(const as_string * s) {
    uint32_t hash = 0;
    int c;
    char * str = s->value;
    while ( (c = *str++) ) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

uint32_t as_string_val_hash(const as_val * v) {
    return as_string_hash((as_string *) v);
}

char * as_string_val_tostring(const as_val * v) {
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
