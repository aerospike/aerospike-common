#include "as_hashmap.h"
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



struct as_hashmap_s {
};

struct as_hashmap_iterator_source_s {
    const as_map * map;
};


as_map * as_hashmap_new(uint32_t capacity, uint32_t block_size, as_val_hash_function hash) {
    as_hashmap * m = (as_hashmap *) malloc(sizeof(as_hashmap));
    return as_map_new(m, &as_hashmap_hooks);
}

static int as_hashmap_free(as_map * l) {
    return 0;
}

static uint32_t as_hashmap_size(const as_map * m) {
    return 0;
}

static int as_hashmap_set(as_map * m, const as_val * k, const as_val * v) {
    return 0;
}

static as_val * as_hashmap_get(const as_map * m, const as_val * k) {
    return NULL;
}

static as_iterator * as_hashmap_iterator(const as_map * m) {
    as_hashmap_iterator_source * source = (as_hashmap_iterator_source *) malloc(sizeof(as_hashmap_iterator_source));
    source->map = m;
    return as_iterator_new(source, &as_hashmap_iterator_hooks);
}

static const bool as_hashmap_iterator_has_next(const as_iterator * i) {
    return false;
}

static const as_val * as_hashmap_iterator_next(as_iterator * i) {
    return NULL;
}

static const int as_hashmap_iterator_free(as_iterator * i) {
    return 0;
}

static const as_iterator_hooks as_hashmap_iterator_hooks = {
    .has_next   = as_hashmap_iterator_has_next,
    .next       = as_hashmap_iterator_next,
    .free       = as_hashmap_iterator_free
};

static const as_map_hooks as_hashmap_hooks = {
    .free       = as_hashmap_free,
    .size       = as_hashmap_size,
    .set        = as_hashmap_set,
    .get        = as_hashmap_get,
    .iterator   = as_hashmap_iterator
};