#pragma once

#include "as_util.h"
#include "as_val.h"

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_rec_s as_rec;
typedef struct as_rec_hooks_s as_rec_hooks;

/**
 * Record Structure
 * Contains a pointer to the source of the record and 
 * hooks that interface with the source.
 *
 * @field source contains record specific data.
 * @field hooks contains the record interface that works with the source.
 */
struct as_rec_s {
    as_val                  _;
    void *                  source;
    const as_rec_hooks *    hooks;
};

/**
 * Record Interface.
 * Provided functions that interface with the records.
 */
struct as_rec_hooks_s {
    int         (* free)(as_rec *);
    as_val *    (* get)(const as_rec *, const char *);
    int         (* set)(const as_rec *, const char *, const as_val *);
    int         (* remove)(const as_rec *, const char *);
    uint32_t    (* ttl)(const as_rec *);
    uint16_t    (* gen)(const as_rec *);
    uint32_t    (* hash)(as_rec *);
};

/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

extern const as_val as_rec_val;

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline as_rec * as_rec_init(as_rec * r, void * source, const as_rec_hooks * hooks) {
    if ( !r ) return r;
    r->_ = as_rec_val;
    r->source = source;
    r->hooks = hooks;
    return r;
}

/**
 * Create a new as_rec backed by source and supported by hooks.
 *
 * @param source the source backing the as_rec.
 * @param hooks the hooks that support the as_rec.
 */
inline as_rec * as_rec_new(void * source, const as_rec_hooks * hooks) {
    as_rec * r = (as_rec *) malloc(sizeof(as_rec));
    return as_rec_init(r, source, hooks);
}

inline int as_rec_destroy(as_rec * r) {
    if ( !r ) return 0;
    r->source = NULL;
    r->hooks = NULL;
    return 0;
}

/**
 * Free the as_rec.
 * This will free the as_rec object, the source and hooks.
 *
 * Proxies to `r->hooks->free(r)`
 *
 * @param r the as_rec to be freed.
 */
inline int as_rec_free(as_rec * r) {
    if ( !r ) return 0;
    as_util_hook(free, 1, r);
    as_rec_destroy(r);
    free(r);
    return 0;
}

inline void * as_rec_source(const as_rec * r) {
    return (r ? r->source : NULL);
}

inline uint32_t as_rec_hash(as_rec * r) {
    return as_util_hook(hash, 0, r);
}

/**
 * Get a bin value by name.
 *
 * Proxies to `r->hooks->get(r, name, value)`
 *
 * @param r the as_rec to read the bin value from.
 * @param name the name of the bin.
 * @param a as_val containing the value in the bin.
 */
inline as_val * as_rec_get(const as_rec * r, const char * name) {
    return as_util_hook(get, NULL, r, name);
}

/**
 * Set the value of a bin.
 *
 * Proxies to `r->hooks->set(r, name, value)`
 *
 * @param r the as_rec to write the bin value to.
 * @param name the name of the bin.
 * @param value the value of the bin.
 */
inline int as_rec_set(const as_rec * r, const char * name, const as_val * value) {
    return as_util_hook(set, 1, r, name, value);
}

/**
 * Rmeove a bin from a record.
 *
 * Proxies to `r->hooks->set(r, name, value)`
 *
 * @param r the record to remove the bin from.
 * @param name the name of the bin to remove.
 */
inline int as_rec_remove(const as_rec * r, const char * name) {
    return as_util_hook(remove, 1, r, name);
}


inline uint32_t as_rec_ttl(const as_rec * r) {
    return as_util_hook(ttl, 0, r);
}

inline uint16_t as_rec_gen(const as_rec * r) {
    return as_util_hook(gen, 0, r);
}

inline as_val * as_rec_toval(const as_rec * r) {
    return (as_val *) r;
}

inline as_rec * as_rec_fromval(const as_val * v) {
    return as_util_fromval(v, AS_REC, as_rec);
}
