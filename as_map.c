#include "as_map.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

static const as_val AS_MAP_VAL;

inline int as_map_hash(as_map * m);
inline int as_map_free(as_map * m);

as_map * as_map_new(void * source, const as_map_hooks * hooks) {
    as_map * m = (as_map *) malloc(sizeof(as_map));
    as_map_init(m, source, hooks);
    return m;
}

int as_map_init(as_map * m, void * source, const as_map_hooks * hooks) {
    m->_ = AS_MAP_VAL;
    m->source = source;
    m->hooks = hooks;
    return 0;
}

static int as_map_val_free(as_val * v) {
    return as_val_type(v) == AS_MAP ? as_map_free((as_map *) v) : 1;
}

static uint32_t as_map_val_hash(as_val * v) {
    return as_val_type(v) == AS_MAP ? as_map_hash((as_map *) v) : 0;
}

static char * as_map_val_tostring(as_val * v) {
    if ( as_val_type(v) != AS_MAP ) return NULL;
    char * str = (char *) malloc(sizeof(char) * 64);
    bzero(str,64);
    strcpy(str,"Map()");
    return str;
}

static const as_val AS_MAP_VAL = {
    .type       = AS_MAP,
    .size       = sizeof(as_map),
    .free       = as_map_val_free, 
    .hash       = as_map_val_hash,
    .tostring   = as_map_val_tostring
};