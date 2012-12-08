/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once

#if ! (defined(MARCH_i686) || defined(MARCH_x86_64))
#if defined(__LP64__) || defined(_LP64)
#define MARCH_x86_64 1
#else
#define MARCH_i686 1
#endif
#endif
