#include "as_list.h"
#include "as_util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void * as_list_source(const as_list *);
extern inline int as_list_free(as_list *);
extern inline uint32_t as_list_hash(as_list *);
extern inline uint32_t as_list_size(as_list *);
extern inline int as_list_append(as_list *, as_val *);
extern inline int as_list_prepend(as_list *, as_val *);
extern inline as_val * as_list_get(const as_list *, const uint32_t);
extern inline int as_list_set(as_list *, const uint32_t, as_val *);
extern inline as_val * as_list_head(const as_list *);
extern inline as_list * as_list_tail(const as_list *);
extern inline as_iterator * as_list_iterator(const as_list *);
extern inline as_val * as_list_toval(const as_list *);
extern inline as_list * as_list_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_list_val_free(as_val *);
static uint32_t as_list_val_hash(as_val *);
static char * as_list_val_tostring(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_val AS_LIST_VAL = {
    .type       = AS_LIST,
    .size       = sizeof(as_list),
    .free       = as_list_val_free, 
    .hash       = as_list_val_hash, 
    .tostring   = as_list_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list * as_list_new(void * source, const as_list_hooks * hooks) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    as_list_init(l, source, hooks);
    return l;
}

int as_list_init(as_list * l, void * source, const as_list_hooks * hooks) {
    l->_ = AS_LIST_VAL;
    l->source = source;
    l->hooks = hooks;
    return 0;
}

static int as_list_val_free(as_val * v) {
    return as_val_type(v) == AS_LIST ? as_list_free((as_list *) v) : 1;
}

static uint32_t as_list_val_hash(as_val * v) {
    return as_val_type(v) == AS_INTEGER ? as_list_hash((as_list *) v) : 0;
}

static char * as_list_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_LIST ) return NULL;

    as_list * l = (as_list *) v;

    char *      buf = NULL;
    uint32_t    blk = 256;
    uint32_t    cap = blk;
    uint32_t    pos = 0;
    bool        sep = false;

    buf = (char *) malloc(sizeof(char) * cap);
    bzero(buf, sizeof(char) * cap);

    strcpy(buf, "List(");
    pos += 5;
    
    as_iterator * i = as_list_iterator(l);
    while ( as_iterator_has_next(i) ) {
        as_val * val = (as_val *) as_iterator_next(i);
        char * valstr = as_val_tostring(val);
        size_t vallen = strlen(valstr);

        if ( pos + vallen + 2 >= cap ) {
            uint32_t adj = vallen+2 > blk ? vallen+2 : blk;
            buf = realloc(buf, sizeof(char) * (cap + adj));
            bzero(buf+cap, sizeof(char)*adj);
            cap += adj;
        }

        if ( sep ) {
            strcpy(buf + pos, ", ");
            pos += 2;
        }

        strncpy(buf + pos, valstr, vallen);
        pos += vallen;
        sep = true;
    }

    strcpy(buf + pos, ")");
    
    return buf;
}
