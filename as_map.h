#ifndef _AS_MAP_H
#define _AS_MAP_H

#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>

typedef struct as_map_s as_map;

as_map * as_map_new();

int as_map_free(as_map *);

as_val * as_map_get(const as_map *, const as_val *);

int as_map_set(as_map *, const as_val *, const as_val *);

as_val * as_map_toval(const as_map *);

as_map * as_map_fromval(const as_val *);

// as_iterator * as_map_iterator(const as_map *);

#endif // _AS_MAP_H