#include "as_map.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

static const as_val AS_MAP_VAL;


as_map * as_map_new(void * source, const as_map_hooks * hooks) {
    as_map * m = (as_map *) malloc(sizeof(as_map));
    m->_ = AS_MAP_VAL;
    m->source = source;
    m->hooks = hooks;
    return m;
}

int as_map_free(as_map * m) {
    if ( m->hooks == NULL ) return 1;
    if ( m->hooks->free == NULL ) return 2;
    return m->hooks->free(m);
}


void * as_map_source(const as_map * m) {
    return m->source;
}

static uint32_t as_map_hash(as_map * m) {
    if ( m->hooks == NULL ) return 1;
    if ( m->hooks->hash == NULL ) return 2;
    return m->hooks->hash(m);
}

uint32_t as_map_size(const as_map * m) {
    if ( m->hooks == NULL ) return 1;
    if ( m->hooks->size == NULL ) return 2;
    return m->hooks->size(m);
}

as_val * as_map_get(const as_map * m, const as_val * k) {
    if ( m->hooks == NULL ) return NULL;
    if ( m->hooks->get == NULL ) return NULL;
    return m->hooks->get(m, k);
}

int as_map_set(as_map * m, const as_val * k, const as_val * v) {
    if ( m->hooks == NULL ) return 1;
    if ( m->hooks->set == NULL ) return 2;
    return m->hooks->set(m, k, v);
}

as_iterator * as_map_iterator(const as_map * m) {
    if ( m->hooks == NULL ) return NULL;
    if ( m->hooks->iterator == NULL ) return NULL;
    return m->hooks->iterator(m);
}

as_val * as_map_toval(const as_map * m) {
    return (as_val *) m;
}

as_map * as_map_fromval(const as_val * v) {
    return as_val_type(v) == AS_MAP ? (as_map *) v : NULL;
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
    .free       = as_map_val_free, 
    .hash       = as_map_val_hash,
    .tostring   = as_map_val_tostring
};