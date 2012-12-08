/*
 *  Citrusleaf Foundation
 *  include/slist.h - Very simple list structure.
 *  Highly memory efficient, but not intended to scale to a large number of
 *  objects.
 *
 *  Copyright 2012 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */

#pragma once
 
#include "cf.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>



#define SLIST_ERR_FOUND -3
#define SLIST_ERR_NOTFOUND -2
#define SLIST_ERR -1
#define SLIST_OK 0

#define SLIST_REDUCE_DELETE 1	// indicate that a delete should be done during the reduction

typedef struct {
	uint32_t	key_sz;
	uint32_t	value_sz;
	uint8_t		resize_factor;
} slist_tdata;


//
// Quirks:
// * The key must be the first bytes of the data.
// * Locking must be handled externally to this library (for now).
// * Only supports up to 255 elements. If you have greater than 255 elements,
//   you should be using a better structure.
//


int slist_create(void *v_this, void *v_tdata);
void slist_destroy(void *v_this);

int slist_put(void *v_this, void *key, void *value, void *v_tdata);
int slist_put_unique(void *v_this, void *key, void *value, void *v_tdata);

int slist_get(void *v_this, void *key, void *value, void *v_tdata);

int slist_delete(void *v_this, void *key, void *v_tdata);
int slist_get_and_delete(void *v_this, void *key, void *value, void *v_tdata);

int slist_reduce(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata);
int slist_reduce_delete(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata);

uint32_t slist_get_size(void *v_this);

void slist_test();
