#pragma once

#include "as_serializer.h"
#include <msgpack.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_serializer * as_msgpack_new();

int as_msgpack_init(as_serializer *);

