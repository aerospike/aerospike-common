/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cf_detail( __UNIT, __fmt, __args...) fprintf(stderr, "DETAIL"__fmt, ## __args)
#define cf_debug( __UNIT, __fmt, __args...) fprintf(stderr, "DEBUG"__fmt, ## __args)
#define cf_info( __UNIT, __fmt, __args...) fprintf(stderr, "INFO"__fmt, ## __args)