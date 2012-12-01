#include "as_hashmap.h"
#include "as_string.h"
#include "as_pair.h"
#include "shash.h"
#include <stdlib.h>

typedef struct as_hashmap_s as_hashmap;
typedef struct as_hashmap_iterator_source_s as_hashmap_iterator_source;

static const as_map_hooks as_hashmap_hooks;
static const as_iterator_hooks as_hashmap_iterator_hooks;

static int as_hashmap_free(as_map *);
static uint32_t as_hashmap_size(const as_map *);
static int as_hashmap_set(as_map *, const as_val *, const as_val *);
static as_val * as_hashmap_get(const as_map *, const as_val *);
static as_iterator * as_hashmap_iterator(const as_map *);

struct as_hashmap_iterator_source_s {
    shash * h;
    shash_elem * curr;
    shash_elem * next;
    uint32_t pos;
    uint32_t size;
};



static uint32_t as_hashmap_hashfn(void * k) {
    return *((uint32_t *) k);
}



as_map * as_hashmap_new(uint32_t capacity) {
    shash * t = NULL;
    shash_create(&t, as_hashmap_hashfn, sizeof(uint32_t), sizeof(as_pair *), capacity, SHASH_CR_MT_BIGLOCK | SHASH_CR_RESIZE);
    return as_map_new(t, &as_hashmap_hooks);
}




static int as_hashmap_free(as_map * m) {
    shash_destroy((shash *) m->source);
    m->source = NULL;
    free(m);
    m = NULL;
    return 0;
}

static uint32_t as_hashmap_hash(as_map * l) {
    return 0;
}

static uint32_t as_hashmap_size(const as_map * m) {
    shash * t = (shash *) m->source;
    return shash_get_size(t);
}

static int as_hashmap_set(as_map * m, const as_val * k, const as_val * v) {
    shash * t = (shash *) m->source;
    uint32_t h = as_val_hash(k);
    as_pair * p = pair(k,v);

    return shash_put(t, &h, &p);
}

static as_val * as_hashmap_get(const as_map * m, const as_val * k) {
    shash *  t = (shash *) m->source;
    uint32_t h = as_val_hash(k);
    as_pair * p = NULL;

    if ( shash_get(t, &h, &p) != SHASH_OK ) {
        return NULL;
    }

    return as_pair_2(p);
}

static as_iterator * as_hashmap_iterator(const as_map * m) {
    if ( m == NULL ) return NULL;
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) malloc(sizeof(as_hashmap_iterator_source));
    source->h = (shash *) as_map_source(m);
    source->curr = NULL;
    source->next = NULL;
    source->size = (uint32_t) source->h->table_len;
    source->pos = 0;
    return as_iterator_new(source, &as_hashmap_iterator_hooks);
}

static const bool as_hashmap_iterator_seek(as_hashmap_iterator_source * source) {
    
    if ( source->pos >= source->size ) return false;

    if ( source->curr ) return true;

    if ( source->next ) {
        if ( source->next->in_use ) {
            source->curr = source->next;
            source->next = source->curr->next;
            if ( !source->next ) source->pos++;
            return true;
        }
        else {
            source->pos++;
            source->next = NULL;
        }
    }

    for( ; source->pos < source->size; source->pos++ ) {
        source->curr = (shash_elem *) (((byte *) source->h->table) + (SHASH_ELEM_SZ(source->h) * source->pos));
        if ( source->curr && source->curr->in_use ) {
            source->next = source->curr->next;
            if ( !source->next ) source->pos++;
            return true;
        }
    }
    
    source->curr = NULL;
    source->next = NULL;
    source->pos = source->size;
    return false;
}

static const bool as_hashmap_iterator_has_next(const as_iterator * i) {
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) as_iterator_source(i);
    return as_hashmap_iterator_seek(source);
}

static const as_val * as_hashmap_iterator_next(as_iterator * i) {
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) as_iterator_source(i);
    
    if ( !as_hashmap_iterator_seek(source) ) return NULL;

    shash *         h   = source->h;
    shash_elem *    e   = source->curr;
    as_pair **      p   = (as_pair **) SHASH_ELEM_VALUE_PTR(h, e);
    
    source->curr = NULL; // consume the value, so we can get the next one.

    return (as_val *) *p;
}

static const int as_hashmap_iterator_free(as_iterator * i) {
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) as_iterator_source(i);
    if ( source ) free(source);
    free(i);
    return 0;
}

static const as_iterator_hooks as_hashmap_iterator_hooks = {
    .has_next   = as_hashmap_iterator_has_next,
    .next       = as_hashmap_iterator_next,
    .free       = as_hashmap_iterator_free
};


static const as_map_hooks as_hashmap_hooks = {
    .free       = as_hashmap_free,
    .hash       = as_hashmap_hash,
    .size       = as_hashmap_size,
    .set        = as_hashmap_set,
    .get        = as_hashmap_get,
    .iterator   = as_hashmap_iterator
};