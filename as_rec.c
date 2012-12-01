#include "as_rec.h"
#include <stdlib.h>
#include <stdio.h>


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void * as_rec_source(const as_rec *);
extern inline int as_rec_free(as_rec *);
extern inline uint32_t as_rec_hash(as_rec *);
extern inline as_val * as_rec_get(const as_rec *, const char *);
extern inline int as_rec_set(const as_rec *, const char *, const as_val *);
extern inline int as_rec_remove(const as_rec *, const char *);
extern inline as_val * as_rec_toval(const as_rec *);
extern inline as_rec * as_rec_fromval(const as_val *);

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int as_rec_val_free(as_val *);
static uint32_t as_rec_val_hash(as_val *);

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

static const as_val AS_REC_VAL = {
    .type       = AS_REC,
    .size       = sizeof(as_rec), 
    .free       = as_rec_val_free, 
    .hash       = as_rec_val_hash,
    .tostring   = NULL
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Create a new as_rec backed by source and supported by hooks.
 *
 * @param source the source backing the as_rec.
 * @param hooks the hooks that support the as_rec.
 */
as_rec * as_rec_new(void * source, const as_rec_hooks * hooks) {
    as_rec * r = (as_rec *) malloc(sizeof(as_rec));
    as_rec_init(r, source, hooks);
    return r;
}

int as_rec_init(as_rec * r, void * source, const as_rec_hooks * hooks) {
    r->_ = AS_REC_VAL;
    r->source = source;
    r->hooks = hooks;
    return 0;
}


static int as_rec_val_free(as_val * v) {
    return as_val_type(v) == AS_REC ? as_rec_free((as_rec *) v) : 1;
}

static uint32_t as_rec_val_hash(as_val * v) {
    return as_val_type(v) == AS_REC ? as_rec_hash((as_rec *) v) : 0;
}
