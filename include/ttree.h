/*
 *  Citrusleaf Foundation
 *  include/ttree.h - T-tree index
 *
 *  Copyright 2012 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */

#pragma once
 
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <cf.h>


#define TTREE_ERR_FOUND -3
#define TTREE_ERR_NOTFOUND -2
#define TTREE_ERR -1
#define TTREE_OK 0

#define TTREE_REDUCE_DELETE 1	// indicate that a delete should be done during the reduction

typedef struct {
	uint32_t	key_sz;
	uint32_t	value_sz;
	uint32_t	max_row_elements;
} ttree_tdata;


//
// Quirks:
// * The key must be the first bytes of the data.
// * Locking must be handled externally to this library (for now).
//


int ttree_create(void *v_this, void *v_tdata);
void ttree_destroy(void *v_this);

int ttree_put(void *v_this, void *key, void *value, void *v_tdata);
int ttree_put_unique(void *v_this, void *key, void *value, void *v_tdata);

int ttree_get(void *v_this, void *key, void *value, void *v_tdata);

int ttree_delete(void *v_this, void *key, void *v_tdata);
int ttree_get_and_delete(void *v_this, void *key, void *value, void *v_tdata);

int ttree_reduce(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata);
int ttree_reduce_delete(void *v_this, cf_index_reduce_fn reduce_fn, void *v_tdata, void *udata);

uint32_t ttree_get_size(void *v_this);

void ttree_test();
