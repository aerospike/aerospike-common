/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once

#include "../cf_queue_priority.h"

extern int cf_queue_priority_reduce_pop(cf_queue_priority *q, void *buf, cf_queue_reduce_fn cb, void *udata);
