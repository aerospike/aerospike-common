#include "as_aerospike.h"
#include <stdlib.h>
#include <cf_alloc.h>
#include "internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_aerospike_create(const as_aerospike *, const as_rec *);
extern inline int as_aerospike_update(const as_aerospike *, const as_rec *);
extern inline int as_aerospike_remove(const as_aerospike *, const as_rec *);
extern inline int as_aerospike_exists(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_log(const as_aerospike *, const char *, const int, const int, const char *);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_init(as_aerospike * a, void * s, const as_aerospike_hooks * h) {
    if ( !a ) return a;
    a->source = s;
    a->hooks = h;
    return a;
}

int as_aerospike_destroy(as_aerospike * a) {
    if ( !a ) return 0;
    a->source = NULL;
    a->hooks = NULL;
    return 0;
}

as_aerospike * as_aerospike_new(void * s, const as_aerospike_hooks * h) {
    as_aerospike * a = (as_aerospike *) cf_rc_alloc(sizeof(as_aerospike));
    return as_aerospike_init(a, s, h);
}

int as_aerospike_free(as_aerospike * a) {
    if ( !a ) return 0;
    LOG("as_aerospike_free: release");
    if ( cf_rc_release(a) > 0 ) return 0;
    as_util_hook(free, 1, a);
    as_aerospike_destroy(a); 
    cf_rc_free(a);
    LOG("as_aerospike_free: free");
    return 0;
}