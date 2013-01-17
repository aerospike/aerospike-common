#pragma once

#include "as_util.h"
#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>
#include <inttypes.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_list_s as_list;
typedef struct as_list_hooks_s as_list_hooks;

typedef void (* as_list_foreach_callback)(as_val *, void *);

struct as_list_s {
    as_val                  _;
    void *                  source;
    const as_list_hooks *   hooks;
};

struct as_list_hooks_s {
    int (* free)(as_list *);
    uint32_t (* hash)(const as_list *);
    uint32_t (* size)(const as_list *);
    int (* append)(as_list *, as_val *);
    int (* prepend)(as_list *, as_val *);
    as_val * (* get)(const as_list *, const uint32_t);
    int (* set)(as_list *, const uint32_t, as_val *);
    as_val * (* head)(const as_list *);
    as_list * (* tail)(const as_list *);
    as_list * (* drop)(const as_list *, uint32_t);
    as_list * (* take)(const as_list *, uint32_t);

    void (* foreach)(const as_list *, void *, as_list_foreach_callback);

    as_iterator * (* iterator)(const as_list *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int as_list_init(as_list *, void *, const as_list_hooks *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_list_destroy(as_list * l) {
    l->source = NULL;
    l->hooks = NULL;
    return 0;
}

inline as_list * as_list_new(void * source, const as_list_hooks * hooks) {
    as_list * l = (as_list *) malloc(sizeof(as_list));
    as_list_init(l, source, hooks);
    return l;
}

inline int as_list_free(as_list * l) {
    return as_util_hook(free, 1, l);
}

inline void * as_list_source(const as_list * l) {
    return l->source;
}

inline uint32_t as_list_hash(as_list * l) {
    return as_util_hook(hash, 0, l);
}

inline uint32_t as_list_size(as_list * l) {
    return as_util_hook(size, 0, l);
}

inline int as_list_append(as_list * l, as_val * v) {
    return as_util_hook(append, 1, l, v);
}

inline int as_list_prepend(as_list * l, as_val * v) {
    return as_util_hook(prepend, 1, l, v);
}

inline as_val * as_list_get(const as_list * l, const uint32_t i) {
    return as_util_hook(get, NULL, l, i);
}

inline int as_list_set(as_list * l, const uint32_t i, as_val * v) {
    return as_util_hook(set, 1, l, i, v);
}

inline as_val * as_list_head(const as_list * l) {
    return as_util_hook(head, NULL, l);
}

inline as_list * as_list_tail(const as_list * l) {
    return as_util_hook(tail, NULL, l);
}

inline as_list * as_list_drop(const as_list * l, uint32_t n) {
    return as_util_hook(drop, NULL, l, n);
}

inline as_list * as_list_take(const as_list * l, uint32_t n) {
    return as_util_hook(take, NULL, l, n);
}

inline void as_list_foreach(const as_list * l, void * context, as_list_foreach_callback callback) {
    as_util_hook(foreach, NULL, l, context, callback);
}

inline as_iterator * as_list_iterator(const as_list * l) {
    return as_util_hook(iterator, NULL, l);
}

inline as_val * as_list_toval(const as_list * l) {
    return (as_val *) l;
}

inline as_list * as_list_fromval(const as_val * v) {
    return as_util_fromval(v, AS_LIST, as_list);
}