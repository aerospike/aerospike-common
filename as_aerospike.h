#pragma once

#include "as_util.h"
#include "as_types.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_aerospike_s as_aerospike;
typedef struct as_aerospike_hooks_s as_aerospike_hooks;

struct as_aerospike_s {
    bool    is_rcalloc;
    void * source;
    const as_aerospike_hooks * hooks;
};

struct as_aerospike_hooks_s {
    void (*destroy)(as_aerospike *);
    int (*rec_create)(const as_aerospike *, const as_rec *);
    int (*rec_update)(const as_aerospike *, const as_rec *);
    int (*rec_remove)(const as_aerospike *, const as_rec *);
    int (*rec_exists)(const as_aerospike *, const as_rec *);
    int (*log)(const as_aerospike *, const char *, const int, const int, const char *);

	// Chunk record related interfaces. Specific to Large Stack Objects
    as_rec *(*crec_create)(const as_aerospike *, const as_rec *);
    as_rec *(*crec_open)(const as_aerospike *, const as_rec *, const as_bytes *);
    int (*crec_update)(const as_aerospike *, const as_rec *, const as_rec *);
    int (*crec_close)(const as_aerospike *, const as_rec *, const as_rec *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_init(as_aerospike *a, void *source, const as_aerospike_hooks *hooks);

as_aerospike * as_aerospike_new(void *source, const as_aerospike_hooks *hooks);

void as_aerospike_destroy(as_aerospike *);


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_aerospike_rec_create(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(rec_create, 1, a, r);
}

inline int as_aerospike_rec_update(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(rec_update, 1, a, r);
}

inline as_rec *as_aerospike_crec_create(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(crec_create, NULL, a, r);
}

inline int as_aerospike_crec_update(const as_aerospike * a, const as_rec * r, const as_rec * cr) {
    return as_util_hook(crec_update, 1, a, r, cr);
}

inline int as_aerospike_crec_close(const as_aerospike * a, const as_rec * r, const as_rec * cr) {
    return as_util_hook(crec_close, 1, a, r, cr);
}

inline as_rec * as_aerospike_crec_open(const as_aerospike * a, const as_rec * r, const as_bytes *bdig) {
    return as_util_hook(crec_open, NULL, a, r, bdig);
}

inline int as_aerospike_rec_exists(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(rec_exists, 1, a, r);
}

inline int as_aerospike_rec_remove(const as_aerospike * a, const as_rec * r) {
    return as_util_hook(rec_remove, 1, a, r);
}

inline int as_aerospike_log(const as_aerospike * a, const char * name, const int line, const int lvl, const char * msg) {
    return as_util_hook(log, 1, a, name, line, lvl, msg);
}
