#pragma once

#include "as_map.h"
#include "as_pair.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct shash_s as_hashmap;
typedef struct as_hashmap_iterator_source_s as_hashmap_iterator_source;

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_map_hooks      as_hashmap_map;
extern const as_iterator_hooks as_hashmap_iterator;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_hashmap *    as_hashmap_new(uint32_t);
int             as_hashmap_free(as_hashmap *);
 
int             as_hashmap_set(as_hashmap * m, const as_val * k, const as_val * v);
as_val *        as_hashmap_get(const as_hashmap * m, const as_val * k);
uint32_t        as_hashmap_size(const as_hashmap *);
int             as_hashmap_clear(as_hashmap *);
