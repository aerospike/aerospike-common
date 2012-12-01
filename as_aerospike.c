#include "as_aerospike.h"
#include <stdlib.h>

as_aerospike * as_aerospike_new(void * source, const as_aerospike_hooks * hooks) {
    as_aerospike * a = (as_aerospike *) malloc(sizeof(as_aerospike));
    as_aerospike_init(a, source, hooks);
    return a;
}