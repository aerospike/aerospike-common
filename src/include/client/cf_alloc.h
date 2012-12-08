/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include "cf_atomic.h"
#include "../cf_alloc.h"

/******************************************************************************
 * ALIASES
 ******************************************************************************/

#define cf_client_rc_count cf_rc_count
#define cf_client_rc_reserve cf_rc_reserve
#define cf_client_rc_release cf_rc_release
#define cf_client_rc_releaseandfree cf_rc_releaseandfree

#define cf_client_rc_alloc(__a) cf_rc_alloc_at(__a, __FILE__, __LINE__)
#define cf_client_rc_free(__a) cf_rc_free_at(__a, __FILE__, __LINE__)
