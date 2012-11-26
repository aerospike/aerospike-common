#include "as_map.h"
#include <stdlib.h>

struct as_map_s {
    as_val _;
};

static const as_val AS_MAP_VAL;
static int as_map_freeval(as_val *);



as_map * as_map_new() {
    as_map * m = (as_map *) malloc(sizeof(as_map));
    m->_ = AS_MAP_VAL;
    return m;
}

int as_map_free(as_map * m) {
    free(m);
    return 0;
}

as_val * as_map_get(const as_map * m, const as_val * key) {
    return NULL;
}

int as_map_set(as_map * m, const as_val * key, const as_val * value) {
    return 0;
}

as_val * as_map_toval(const as_map * m) {
    return (as_val *) m;
}

as_map * as_map_fromval(const as_val * v) {
    return as_val_type(v) == AS_MAP ? (as_map *) v : NULL;
}

static int as_map_freeval(as_val * v) {
    return as_val_type(v) == AS_MAP ? as_map_free((as_map *) v) : 1;
}

static const as_val AS_MAP_VAL = {AS_MAP, as_map_freeval};