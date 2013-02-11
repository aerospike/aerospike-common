#pragma once

#include <msgpack.h>

#include "as_serializer.h"


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_serializer * as_msgpack_new();

int as_msgpack_init(as_serializer *);

int as_msgpack_pack_val(msgpack_packer *, as_val *);
int as_msgpack_object_to_val(msgpack_object *, as_val **);
