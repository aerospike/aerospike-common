#pragma once

#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>

typedef struct as_map_s as_map;
typedef struct as_map_entry_s as_map_entry;
typedef struct as_map_hooks_s as_map_hooks;


struct as_map_s {
    as_val                  _;
    void *                  source;
    const as_map_hooks *    hooks;
};

struct as_map_entry_s {
    as_val * key;
    as_val * value;
};

struct as_map_hooks_s {
    int (*free)(as_map *);
    uint32_t (*hash)(as_map *);

    uint32_t (* size)(const as_map *);
    int (* set)(as_map *, const as_val *, const as_val *);
    as_val * (* get)(const as_map *, const as_val *);
    as_iterator * (*iterator)(const as_map *);
};



as_map * as_map_new(void *, const as_map_hooks *);

int as_map_free(as_map *);

void * as_map_source(const as_map *);

uint32_t as_map_size(const as_map *);

int as_map_set(as_map *, const as_val *, const as_val *);

as_val * as_map_get(const as_map *, const as_val *);

as_val * as_map_toval(const as_map *);

as_map * as_map_fromval(const as_val *);

as_iterator * as_map_iterator(const as_map *);
