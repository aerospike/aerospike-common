/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include "../cf_digest.h"

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void cf_digest_string(cf_digest *digest, char* output);
void cf_digest_dump(cf_digest *digest);