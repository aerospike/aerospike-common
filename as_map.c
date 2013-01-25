#include "as_map.h"
#include "as_pair.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <cf_alloc.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void *        as_map_source(const as_map *);

extern inline uint32_t      as_map_size(const as_map *);
extern inline as_val *      as_map_get(const as_map *, const as_val *);
extern inline int           as_map_set(as_map *, const as_val *, const as_val *);
extern inline int           as_map_clear(as_map *);
extern inline as_iterator * as_map_iterator(const as_map *);

extern inline int           as_map_hash(as_map *);
extern inline as_val *      as_map_toval(const as_map *);
extern inline as_map *      as_map_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int      as_map_val_free(as_val *);
static uint32_t as_map_val_hash(as_val *);
static char *   as_map_val_tostring(as_val *);

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

const as_val as_map_val = {
    .type       = AS_MAP,
    .size       = sizeof(as_map),
    .free       = as_map_val_free, 
    .hash       = as_map_val_hash,
    .tostring   = as_map_val_tostring
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_map * as_map_init(as_map * m, void * source, const as_map_hooks * hooks) {
    if ( !m ) return m;
    m->_ = as_map_val;
    m->source = source;
    m->hooks = hooks;
    return m;
}

as_map * as_map_new(void * source, const as_map_hooks * hooks) {
    as_map * m = (as_map *) cf_rc_alloc(sizeof(as_map));
    return as_map_init(m, source, hooks);
}

int as_map_destroy(as_map * m) {
    if ( !m ) return 0;
    m->source = NULL;
    m->hooks = NULL;
    return 0;
}

/**
 * Free sequence:
 * 1. Call the hook designated for freeing the source.
 * 2. NULL-ify members
 * 3. Free the map
 */
int as_map_free(as_map * m) {
    if ( !m ) return 0;
    if ( cf_rc_release(m) > 0 ) return 0;
    as_util_hook(free, 1, m);
    as_map_destroy(m);
    cf_rc_free(m);
    return 0;
}

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_map_val_free(as_val * v) {
    return as_val_type(v) == AS_MAP ? as_map_free((as_map *) v) : 1;
}

static uint32_t as_map_val_hash(as_val * v) {
    return as_val_type(v) == AS_MAP ? as_map_hash((as_map *) v) : 0;
}

static char * as_map_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_MAP ) return NULL;

    as_map * m = (as_map *) v;

    char *      buf = NULL;
    uint32_t    blk = 256;
    uint32_t    cap = blk;
    uint32_t    pos = 0;
    bool        sep = false;

    buf = (char *) malloc(sizeof(char) * cap);
    bzero(buf, sizeof(char) * cap);

    strcpy(buf, "Map(");
    pos += 4;
    
    as_iterator * i = as_map_iterator(m);
    while ( as_iterator_has_next(i) ) {
        as_pair * pair = (as_pair *) as_iterator_next(i);

        char * keystr = as_val_tostring(as_pair_1(pair));
        size_t keylen = strlen(keystr);

        char * valstr = as_val_tostring(as_pair_2(pair));
        size_t vallen = strlen(valstr);

        if ( pos + keylen + 2 + vallen + 2 >= cap ) {
            uint32_t adj = keylen+2+vallen+2 > blk ? keylen+2+vallen+2 : blk;
            buf = realloc(buf, sizeof(char) * (cap + adj));
            bzero(buf+cap, sizeof(char)*adj);
            cap += adj;
        }

        if ( sep ) {
            strcpy(buf + pos, ", ");
            pos += 2;
        }

        strncpy(buf + pos, keystr, keylen);
        strcpy(buf + pos + keylen, "->");
        strncpy(buf + pos + keylen + 2, valstr, vallen);
        pos += keylen + 2 + vallen;
        sep = true;

        free(keystr);
        keystr = NULL;
        free(valstr);
        valstr = NULL;
    }

    as_iterator_free(i);

    strcpy(buf + pos, ")");
    
    return buf;
}
