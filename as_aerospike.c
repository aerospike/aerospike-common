#include "as_aerospike.h"
#include <stdlib.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_aerospike_init(as_aerospike * a, void * s, const as_aerospike_hooks * h);
extern inline int as_aerospike_free(as_aerospike * a);
extern inline int as_aerospike_create(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_update(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_exists(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_remove(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_log(const as_aerospike * a, const char * name, const int line, const int lvl, const char * msg);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_new(void * source, const as_aerospike_hooks * hooks) {
    as_aerospike * a = (as_aerospike *) malloc(sizeof(as_aerospike));
    as_aerospike_init(a, source, hooks);
    return a;
}
