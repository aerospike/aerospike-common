
#include "as_serializer.h"

#include "internal.h"

/******************************************************************************
 * åFUNCTIONS
 ******************************************************************************/

as_serializer * as_serializer_init(as_serializer * s, const void * source, const as_serializer_hooks * hooks) {
    s->is_malloc = false;
    s->hooks = hooks;
    return s;
}

as_serializer * as_serializer_new(const void * source, const as_serializer_hooks * hooks) {
    as_serializer * s = (as_serializer *) malloc(sizeof(as_serializer));
    s->is_malloc = true;
    s->hooks = hooks;
    return s;
}

void as_serializer_destroy(as_serializer * s) {
    if (s->is_malloc == true) free(s);
}

int as_serializer_serialize(as_serializer * s, as_val * v, as_buffer * b) {
    return as_util_hook(serialize, 1, s, v, b);
}

int as_serializer_deserialize(as_serializer * s, as_buffer * b, as_val ** v) {
    return as_util_hook(deserialize, 1, s, b, v);
}


