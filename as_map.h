#pragma once

#include <stdbool.h>

#include "as_internal.h"

#include "as_util.h"
#include "as_val.h"
#include "as_iterator.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_map_s as_map;
typedef struct as_map_hooks_s as_map_hooks;

struct as_map_hooks_s {
    void         (* destroy)(as_map *);
    
    uint32_t    (* hash)(const as_map *);
    
    uint32_t    (* size)(const as_map *);
    int         (* set)(as_map *, const as_val *, const as_val *);
    as_val *    (* get)(const as_map *, const as_val *);
    int         (* clear)(as_map *);

    as_iterator * (*iterator_init)(const as_map *, as_iterator *);
    as_iterator * (*iterator_new)(const as_map *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
 
void          as_map_destroy(as_map *);
void		  as_map_val_destroy(as_val *);

uint32_t	  as_map_val_hash(const as_val *);
char *		  as_map_val_tostring(const as_val *);

//******************************************************************************
//* INLINE FUNCTIONS
//******************************************************************************

inline uint32_t as_map_size(const as_map * m) {
    return as_util_hook(size, 0, m);
}

inline as_val * as_map_get(const as_map * m, const as_val * k) {
    return as_util_hook(get, NULL, m, k);
}

inline int as_map_set(as_map * m, const as_val * k, const as_val * v) {
    return as_util_hook(set, 1, m, k, v);
}

inline int as_map_clear(as_map * m) {
    return as_util_hook(clear, 1, m);
}

inline as_iterator * as_map_iterator_init(as_iterator *i, const as_map * m) {
    return as_util_hook(iterator_init, NULL, m, i);
}

inline as_iterator * as_map_iterator_new(const as_map * m) {
    return as_util_hook(iterator_new, NULL, m);
}

inline int as_map_hash(as_map * m) {
    return as_util_hook(hash, 0, m);
}

inline as_val * as_map_toval(const as_map * m) {
    return (as_val *) m;
}

inline as_map * as_map_fromval(const as_val * v) {
    return as_util_fromval(v, AS_MAP, as_map);
}
