#pragma once

#include "as_util.h"
#include "as_types.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_aerospike_s as_aerospike;
typedef struct as_aerospike_hooks_s as_aerospike_hooks;

struct as_aerospike_s {
    void * source;
    const as_aerospike_hooks * hooks;
};

struct as_aerospike_hooks_s {
    int (*free)(as_aerospike *);
    int (*create)(const as_aerospike *, const as_rec *);
    int (*update)(const as_aerospike *, const as_rec *);
    int (*remove)(const as_aerospike *, const as_rec *);
    int (*exists)(const as_aerospike *, const as_rec *);
    int (*log)(const as_aerospike *, const char *, const int, const int, const char *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_new(void *, const as_aerospike_hooks *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_aerospike_init(as_aerospike * a, void * s, const as_aerospike_hooks * h) {
    a->source = s;
    a->hooks = h;
    return 0;
}

inline int as_aerospike_free(as_aerospike * a) {
    return as_util_hook(free, 1, a);
}

inline int as_aerospike_create(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(create, 1, a, r);
}

inline int as_aerospike_update(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(update, 1, a, r);
}

inline int as_aerospike_remove(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(remove, 1, a, r);
}

inline int as_aerospike_log(const as_aerospike * a, const char * name, const int line, const int lvl, const char * msg) {
    return as_util_hook(log, 1, a, name, line, lvl, msg);
}
