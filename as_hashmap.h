#pragma once

#include "as_internal.h"

#include "as_map.h"
#include "as_pair.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_hashmap_source_s as_hashmap_source;
typedef struct as_hashmap_iterator_source_s as_hashmap_iterator_source;

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_map_hooks      as_hashmap_map;
extern const as_iterator_hooks as_hashmap_iterator;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_map *    as_hashmap_init(as_map *, uint32_t);
as_map *    as_hashmap_new(uint32_t);

void             as_hashmap_destroy(as_map *);
void             as_hashmap_val_destroy(as_val *);

uint32_t		as_hashmap_hash(const as_map *m);
int             as_hashmap_set(as_map * m, const as_val * k, const as_val * v);
as_val *        as_hashmap_get(const as_map * m, const as_val * k);
uint32_t        as_hashmap_size(const as_map *);
int             as_hashmap_clear(as_map *);

as_iterator * as_hashmap_iterator_init(const as_map * m, as_iterator *i );
as_iterator * as_hashmap_iterator_new(const as_map * m);
bool as_hashmap_iterator_has_next(const as_iterator * i);
as_val * as_hashmap_iterator_next(as_iterator * i);
void as_hashmap_iterator_destroy(as_iterator * i);



