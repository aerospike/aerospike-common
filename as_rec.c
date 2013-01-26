#include "as_rec.h"
#include <stdlib.h>
#include <stdio.h>
#include <cf_alloc.h>
#include "internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void *    as_rec_source(const as_rec *);
extern inline uint32_t  as_rec_hash(as_rec *);

extern inline as_val *  as_rec_get(const as_rec *, const char *);
extern inline int       as_rec_set(const as_rec *, const char *, const as_val *);
extern inline int       as_rec_remove(const as_rec *, const char *);

extern inline uint32_t  as_rec_ttl(const as_rec *);
extern inline uint16_t  as_rec_gen(const as_rec *);

extern inline as_val *  as_rec_toval(const as_rec *);
extern inline as_rec *  as_rec_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_rec_val_free(as_val *);
static uint32_t as_rec_val_hash(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

const as_val as_rec_val = {
    .type       = AS_REC,
    .size       = sizeof(as_rec), 
    .free       = as_rec_val_free, 
    .hash       = as_rec_val_hash,
    .tostring   = NULL
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_rec * as_rec_init(as_rec * r, void * source, const as_rec_hooks * hooks) {
    if ( !r ) return r;
    r->_ = as_rec_val;
    r->source = source;
    r->hooks = hooks;
    return r;
}

int as_rec_destroy(as_rec * r) {
    if ( !r ) return 0;
    r->source = NULL;
    r->hooks = NULL;
    return 0;
}

/**
 * Create a new as_rec backed by source and supported by hooks.
 *
 * @param source the source backing the as_rec.
 * @param hooks the hooks that support the as_rec.
 */
as_rec * as_rec_new(void * source, const as_rec_hooks * hooks) {
    as_rec * r = (as_rec *) cf_rc_alloc(sizeof(as_rec));
    return as_rec_init(r, source, hooks);
}

/**
 * Free the as_rec.
 * This will free the as_rec object, the source and hooks.
 *
 * Proxies to `r->hooks->free(r)`
 *
 * @param r the as_rec to be freed.
 */
int as_rec_free(as_rec * r) {
    if ( !r ) return 0;
    LOG("as_rec_free: release");
    if ( cf_rc_release(r) > 0 ) return 0;
    as_util_hook(free, 1, r);
    as_rec_destroy(r);
    cf_rc_free(r);
    LOG("as_rec_free: free");
    return 0;
}


/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_rec_val_free(as_val * v) {
    return as_val_type(v) == AS_REC ? as_rec_free((as_rec *) v) : 1;
}

static uint32_t as_rec_val_hash(as_val * v) {
    return as_val_type(v) == AS_REC ? as_rec_hash((as_rec *) v) : 0;
}
