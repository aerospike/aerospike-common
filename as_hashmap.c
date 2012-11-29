#include "as_hashmap.h"
#include "as_string.h"
#include "shash.h"
#include <stdlib.h>

typedef struct as_hashmap_s as_hashmap;

static const as_map_hooks as_hashmap_hooks;

static int as_hashmap_free(as_map *);
static uint32_t as_hashmap_size(const as_map *);
static int as_hashmap_set(as_map *, const as_val *, const as_val *);
static as_val * as_hashmap_get(const as_map *, const as_val *);


struct as_hashmap_iterator_source_s {
    const as_map * map;
};

static uint32_t as_hashmap_hashfn(void * k) {
    return as_val_hash((as_val *) k);
}

as_map * as_hashmap_new(uint32_t capacity) {
    shash * t = NULL;
    shash_create(&t, as_hashmap_hashfn, sizeof(as_val), sizeof(as_val *), capacity, SHASH_CR_MT_BIGLOCK);
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
    return shash_put(t, (void *) k, (void *) &v);
}

static as_val * as_hashmap_get(const as_map * m, const as_val * k) {
    shash *  t = (shash *) m->source;
    as_val * v = NULL;

    if ( shash_get(t, (void *) k, (void *) &v) != SHASH_OK ) {
        return NULL;
    }

    return v;
}

static const as_map_hooks as_hashmap_hooks = {
    .free       = as_hashmap_free,
    .hash       = as_hashmap_hash,
    .size       = as_hashmap_size,
    .set        = as_hashmap_set,
    .get        = as_hashmap_get
};