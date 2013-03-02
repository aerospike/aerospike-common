#include <stdlib.h>
#include <stdio.h>
#include <cf_alloc.h>

#include "as_rec.h"
#include "as_bytes.h"


/******************************************************************************
 * INLINES
 ******************************************************************************/

extern inline void * as_rec_source(const as_rec * r);
extern inline uint32_t as_rec_hash(as_rec * r) ;
extern inline void as_rec_destroy(as_rec *r);
extern inline as_val * as_rec_get(const as_rec * r, const char * name) ;
extern inline int as_rec_set(const as_rec * r, const char * name, const as_val * value) ;
extern inline int as_rec_remove(const as_rec * r, const char * name) ;
extern inline uint32_t as_rec_ttl(const as_rec * r);
extern inline uint16_t as_rec_gen(const as_rec * r) ;
extern inline as_bytes *as_rec_digest(const as_rec * r) ;
extern inline uint16_t as_rec_numbins(const as_rec * r) ;
extern inline as_val * as_rec_toval(const as_rec * r) ;
extern inline as_rec * as_rec_fromval(const as_val * v) ;

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_rec * as_rec_init(as_rec * r, void * source, const as_rec_hooks * hooks) {
    if ( !r ) return r;
    as_val_init(&r->_, AS_REC, false /*is_malloc*/);
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
as_rec * as_rec_new(void * source, const as_rec_hooks * hooks) {
    as_rec * r = (as_rec *) malloc(sizeof(as_rec));
    as_val_init(&r->_, AS_REC, true /*is_malloc*/);
    r->source = source;
    r->hooks = hooks;
    return r;
}


void as_rec_val_destroy(as_val *v) {
    as_rec *r = (as_rec *) v;
    as_rec_destroy(r);
	return;
}

uint32_t as_rec_val_hash(const as_val * v) {
    return as_rec_hash((as_rec *) v);
}

char *as_rec_val_tostring(const as_val *v) {
    return (strdup("[ REC ]"));
}
