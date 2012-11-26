#include "as_aerospike.h"
#include <stdlib.h>

as_aerospike * as_aerospike_new(void * source, const as_aerospike_hooks * hooks) {
    as_aerospike * a = (as_aerospike *) malloc(sizeof(as_aerospike));
    a->source = source;
    a->hooks = hooks;
    return a;
}

int as_aerospike_free(as_aerospike * a) {
    if ( !a ) return 1;
    if ( !a->hooks ) return 2;
    if ( !a->hooks->free ) return 3;
    return a->hooks->free(a);
}

void * as_aerospike_source(as_aerospike * a) {
    if ( !a ) return NULL;
    return a->source;
}

// as_rec * as_aerospike_get(as_aerospike * a, const char * ns, const char * set, const char * key) {
//     if ( !a && !a->hooks && !a->hooks->get ) return NULL;
//     return a->hooks->get(a, ns, set, key);
// }

// int as_aerospike_put(as_aerospike * a, const char * ns, const char * set, const char * key, as_map * bins) {
//     if ( !a ) return 1;
//     if ( !a->hooks ) return 2;
//     if ( !a->hooks->put ) return 3;
//     return a->hooks->put(a, ns, set, key, bins);
// }

int as_aerospike_create(as_aerospike * a, as_rec * r) {
    if ( !a ) return 1;
    if ( !a->hooks ) return 2;
    if ( !a->hooks->update ) return 3;
    return a->hooks->create(a, r);
}

int as_aerospike_update(as_aerospike * a, as_rec * r) {
    if ( !a ) return 1;
    if ( !a->hooks ) return 2;
    if ( !a->hooks->update ) return 3;
    return a->hooks->update(a, r);
}

int as_aerospike_remove(as_aerospike * a, as_rec * r) {
    if ( !a ) return 1;
    if ( !a->hooks ) return 2;
    if ( !a->hooks->remove ) return 3;
    return a->hooks->remove(a, r);
}

int as_aerospike_log(as_aerospike * a, const char * name, int line, int lvl, const char * msg) {
    if ( !a ) return 1;
    if ( !a->hooks ) return 2;
    if ( !a->hooks->log ) return 3;
    return a->hooks->log(a, name, line, lvl, msg);
}
