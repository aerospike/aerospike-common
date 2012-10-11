/*
 * Copyright 2012 Aerospike. All rights reserved.
 */
#pragma once

#if ! (defined(MARCH_i686) || defined(MARCH_x86_64))
#if defined(__LP64__) || defined(_LP64)
#define MARCH_x86_64 1
#else
#define MARCH_i686 1
#endif
#endif
