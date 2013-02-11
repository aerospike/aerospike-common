#pragma once

#include <inttypes.h>

#include "as_internal.h"

#include "as_util.h"
#include "as_types.h"
#include "as_buffer.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_serializer_s as_serializer;
typedef struct as_serializer_hooks_s as_serializer_hooks;

struct as_serializer_hooks_s {
    int (*destroy)(as_serializer *);
    int (*serialize)(as_serializer *, as_val *, as_buffer *);
    int (*deserialize)(as_serializer *, as_buffer *, as_val **);
};

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_serializer_init(as_serializer * s, const void * source, const as_serializer_hooks * hooks) {
    s->is_malloc = false;
    s->hooks = hooks;
    return 0;
}

inline as_serializer * as_serializer_new(const void * source, const as_serializer_hooks * hooks) {
    as_serializer * s = (as_serializer *) malloc(sizeof(as_serializer));
    s->is_malloc = true;
    s->hooks = hooks;
    return s;
}

inline void as_serializer_destroy(as_serializer * s) {
    if (s->is_malloc == true) free(s);
}

inline int as_serializer_serialize(as_serializer * s, as_val * v, as_buffer * b) {
    return as_util_hook(serialize, 1, s, v, b);
}

inline int as_serializer_deserialize(as_serializer * s, as_buffer * b, as_val ** v) {
    return as_util_hook(deserialize, 1, s, b, v);
}
