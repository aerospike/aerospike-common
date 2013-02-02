#pragma once

#include <inttypes.h>

#include "as_util.h"
#include "as_types.h"
#include "as_buffer.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_serializer_s as_serializer;
typedef struct as_serializer_hooks_s as_serializer_hooks;

struct as_serializer_hooks_s {
    void (*destroy)(as_serializer *);
    int (*serialize)(as_serializer *, as_val *, as_buffer *);
    int (*deserialize)(as_serializer *, as_buffer *, as_val **);
};


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_serializer * as_serializer_init(as_serializer *, const void *source, const as_serializer_hooks *);
as_serializer * as_serializer_new(const void *source, const as_serializer_hooks *);

void as_serializer_destroy(as_serializer *);

int as_serializer_serialize(as_serializer *, as_val *, as_buffer *);
int as_serializer_deserialize(as_serializer *, as_buffer *, as_val **);
