#pragma once

#include "as_util.h"
#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_map_s as_map;
typedef struct as_map_entry_s as_map_entry;
typedef struct as_map_hooks_s as_map_hooks;

struct as_map_s {
    as_val                  _;
    void *                  source;
    const as_map_hooks *    hooks;
};

struct as_map_hooks_s {
    int (*free)(as_map *);
    uint32_t (*hash)(const as_map *);
    uint32_t (* size)(const as_map *);
    int (* set)(as_map *, const as_val *, const as_val *);
    as_val * (* get)(const as_map *, const as_val *);
    as_iterator * (*iterator)(const as_map *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_map_init(as_map *, void *, const as_map_hooks *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_map_destroy(as_map * m) {
    m->source = NULL;
    m->hooks = NULL;
    return 0;
}

inline as_map * as_map_new(void * source, const as_map_hooks * hooks) {
    as_map * m = (as_map *) malloc(sizeof(as_map));
    as_map_init(m, source, hooks);
    return m;
}

inline int as_map_free(as_map * m) {
    return as_util_hook(free, 1, m);
}

inline void * as_map_source(const as_map * m) {
    return m->source;
}

inline int as_map_hash(as_map * m) {
    return as_util_hook(hash, 0, m);
}

inline uint32_t as_map_size(const as_map * m) {
    return as_util_hook(size, 0, m);
}

inline as_val * as_map_get(const as_map * m, const as_val * k) {
    return as_util_hook(get, NULL, m, k);
}

inline int as_map_set(as_map * m, const as_val * k, const as_val * v) {
    return as_util_hook(set, 1, m, k, v);
}

inline as_iterator * as_map_iterator(const as_map * m) {
    return as_util_hook(iterator, NULL, m);
}

inline as_val * as_map_toval(const as_map * m) {
    return (as_val *) m;
}

inline as_map * as_map_fromval(const as_val * v) {
    return as_util_fromval(v, AS_MAP, as_map);
}