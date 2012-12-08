/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include "cf_log.h"

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define cf_error(__fmt, __args...) \
	if (CF_ERROR <= G_LOG_LEVEL) {(*G_LOG_CB)(CF_ERROR, __fmt, ## __args);}

#define cf_warn(__fmt, __args...) \
	if (CF_WARN <= G_LOG_LEVEL) {(*G_LOG_CB)(CF_WARN, __fmt, ## __args);}

#define cf_info(__fmt, __args...) \
	if (CF_INFO <= G_LOG_LEVEL) {(*G_LOG_CB)(CF_INFO, __fmt, ## __args);}

#define cf_debug(__fmt, __args...) \
	if (CF_DEBUG <= G_LOG_LEVEL) {(*G_LOG_CB)(CF_DEBUG, __fmt, ## __args);}
