#pragma once

#include "as_util.h"
#include "as_types.h"

/******************************************************************************
 *
 * TYPE DECLARATIONS
 * 
 ******************************************************************************/

typedef struct as_aerospike_s as_aerospike;

typedef struct as_aerospike_hooks_s as_aerospike_hooks;

/******************************************************************************
 *
 * TYPE DEFINITIONS
 * 
 ******************************************************************************/

struct as_aerospike_s {
    void * source;
    const as_aerospike_hooks * hooks;
};

struct as_aerospike_hooks_s {
    int (*create)(as_aerospike *, as_rec *);
    int (*update)(as_aerospike *, as_rec *);
    int (*remove)(as_aerospike *, as_rec *);
    int (*log)(as_aerospike *, const char *, int, int, const char *);
    int (*free)(as_aerospike *);
};

/******************************************************************************
 *
 * FUNCTION DECLARATIONS
 * 
 ******************************************************************************/

as_aerospike * as_aerospike_new(void *, const as_aerospike_hooks *);

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS – HOOKS
 * 
 ******************************************************************************/

inline int as_aerospike_free(as_aerospike * a) {
    return as_util_hook(free, 1, a);
}

inline int as_aerospike_create(as_aerospike * a, as_rec * r) {
    return as_util_hook(create, 1, a, r);
}

inline int as_aerospike_update(as_aerospike * a, as_rec * r) {
    return as_util_hook(update, 1, a, r);
}

inline int as_aerospike_remove(as_aerospike * a, as_rec * r) {
    return as_util_hook(remove, 1, a, r);
}

inline int as_aerospike_log(as_aerospike * a, const char * name, int line, int lvl, const char * msg) {
    return as_util_hook(log, 1, a, name, line, lvl, msg);
}
