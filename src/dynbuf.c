/*
 *  Citrusleaf Foundation
 *  src/string.c - string helper functions
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cf.h"

//
// Make sure the buf has enough bytes for whatever you're up to

int
cf_dyn_buf_reserve(cf_dyn_buf *db, size_t	sz)
{
	// see if we need more space
	if ( db->alloc_sz - db->used_sz < sz ) {

		size_t new_sz = db->alloc_sz + sz;
		int backoff;
		if (new_sz < (1024 * 8))
			backoff = 1024;
		else if (new_sz < (1024 * 32))
			backoff = (1024 * 4);
		else if (new_sz < (1024 * 128))
			backoff = (1024 * 32);
		else
			backoff = (1024 * 256);
		
		new_sz = new_sz + (backoff - (new_sz % backoff));

		uint8_t	*_t;
		if (db->is_stack) {
			_t = malloc(new_sz);
			if (!_t)	return(-1);
			memcpy(_t, db->buf, db->used_sz);
			db->is_stack = false;
			db->buf = _t;
		}
		else {
			_t = realloc(db->buf, new_sz);
			if (!_t)	return(-1);
			db->buf = _t;
		}
		db->alloc_sz = new_sz;
	}
	return(0);
}



int
cf_dyn_buf_append_buf(cf_dyn_buf *db, uint8_t *buf, size_t sz)
{
	if (0 != cf_dyn_buf_reserve(db, sz))
		return(-1);
	memcpy(&db->buf[db->used_sz], buf, sz);
	db->used_sz += sz;
	return(0);
}

int
cf_dyn_buf_append_string(cf_dyn_buf *db, char *s)
{
	size_t	len = strlen(s);
	if (0 != cf_dyn_buf_reserve(db, len))
		return(-1);
	memcpy(&db->buf[db->used_sz], s, len);
	db->used_sz += len;
	return ( 0 );
}

int
cf_dyn_buf_append_char (cf_dyn_buf *db, char c)
{
	if (0 != cf_dyn_buf_reserve(db, 1))
		return(-1);
	db->buf[db->used_sz] = (uint8_t) c;
	db->used_sz++;
	return( 0 );
}

// this can be done even faster!

int
cf_dyn_buf_append_int (cf_dyn_buf *db, int i)
{
	// overreserving isn't a crime
	if (0 != cf_dyn_buf_reserve(db, 12))
		return(-1);
	
	db->used_sz += cf_str_itoa(i, (char *) &db->buf[db->used_sz], 10);
	return( 0 );
}

int
cf_dyn_buf_append_uint64_x (cf_dyn_buf *db, uint64_t i)
{
	// overreserving isn't a crime
	if (0 != cf_dyn_buf_reserve(db, 18))
		return(-1);
	
	db->used_sz += cf_str_itoa_u64(i, (char *) &db->buf[db->used_sz], 16);
	return( 0 );
}

int
cf_dyn_buf_append_uint64 (cf_dyn_buf *db, uint64_t i)
{
	// overreserving isn't a crime
	if (0 != cf_dyn_buf_reserve(db, 12))
		return(-1);
	
	db->used_sz += cf_str_itoa_u64(i, (char *) &db->buf[db->used_sz], 10);
	return( 0 );
}

int
cf_dyn_buf_append_uint32 (cf_dyn_buf *db, uint32_t i)
{
	// overreserving isn't a crime
	if (0 != cf_dyn_buf_reserve(db, 12))
		return(-1);
	
	db->used_sz += cf_str_itoa_u32(i, (char *) &db->buf[db->used_sz], 10);
	return( 0 );
}


int
cf_dyn_buf_chomp(cf_dyn_buf *db)
{
	if (db->used_sz > 0)
		db->used_sz--;
	return(0);
}

char *
cf_dyn_buf_strdup(cf_dyn_buf *db)
{
	if (db->used_sz == 0)	return(0);
	char *r = malloc(db->used_sz+1);
	if (!r)	return(0);
	memcpy(r, db->buf, db->used_sz);
	r[db->used_sz] = 0;
	return(r);
}

	

void
cf_dyn_buf_free(cf_dyn_buf *db)
{
	if (db->is_stack == false)
		free(db->buf);
}

//
// Make sure the buf has enough bytes for whatever you're up to

int
cf_buf_builder_reserve(cf_buf_builder **bb_r, size_t	sz)
{
	cf_buf_builder *bb = *bb_r;
	
	// see if we need more space
	if ( bb->alloc_sz - bb->used_sz < sz ) {

		size_t new_sz = bb->alloc_sz + sz + sizeof(cf_buf_builder);
		int backoff;
		if (new_sz < (1024 * 8))
			backoff = 1024;
		else if (new_sz < (1024 * 32))
			backoff = (1024 * 4);
		else if (new_sz < (1024 * 128))
			backoff = (1024 * 32);
		else
			backoff = (1024 * 256);
		
		new_sz = new_sz + (backoff - (new_sz % backoff));

		bb = realloc(bb, new_sz);
		if (!bb)	return(-1);
		bb->alloc_sz = new_sz - sizeof(cf_buf_builder);
		*bb_r = bb;
	}
	return(0);
}



int
cf_buf_builder_append_buf(cf_buf_builder **bb_r, uint8_t *buf, size_t sz)
{
	if (0 != cf_buf_builder_reserve(bb_r, sz))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	memcpy(&bb->buf[bb->used_sz], buf, sz);
	bb->used_sz += sz;
	return(0);
}

int
cf_buf_builder_append_string(cf_buf_builder **bb_r, char *s)
{
	size_t	len = strlen(s);
	if (0 != cf_buf_builder_reserve(bb_r, len))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	memcpy(&bb->buf[bb->used_sz], s, len);
	bb->used_sz += len;
	return ( 0 );
}

int
cf_buf_builder_append_char (cf_buf_builder **bb_r, char c)
{
	if (0 != cf_buf_builder_reserve(bb_r, 1))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	bb->buf[bb->used_sz] = (uint8_t) c;
	bb->used_sz++;
	return( 0 );
}

// this can be done even faster!

int
cf_buf_builder_append_int (cf_buf_builder **bb_r, int i)
{
	// overreserving isn't a crime
	if (0 != cf_buf_builder_reserve(bb_r, 12))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	bb->used_sz += cf_str_itoa(i, (char *) &bb->buf[bb->used_sz], 10);
	return( 0 );
}

int
cf_buf_builder_append_uint64_x (cf_buf_builder **bb_r, uint64_t i)
{
	// overreserving isn't a crime
	if (0 != cf_buf_builder_reserve(bb_r, 18))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	bb->used_sz += cf_str_itoa_u64(i, (char *) &bb->buf[bb->used_sz], 16);
	return( 0 );
}

int
cf_buf_builder_append_uint64 (cf_buf_builder **bb_r, uint64_t i)
{
	// overreserving isn't a crime
	if (0 != cf_buf_builder_reserve(bb_r, 12))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	bb->used_sz += cf_str_itoa_u64(i, (char *) &bb->buf[bb->used_sz], 10);
	return( 0 );
}

int
cf_buf_builder_append_uint32 (cf_buf_builder **bb_r, uint32_t i)
{
	// overreserving isn't a crime
	if (0 != cf_buf_builder_reserve(bb_r, 12))
		return(-1);
	cf_buf_builder *bb = *bb_r;
	bb->used_sz += cf_str_itoa_u32(i, (char *) &bb->buf[bb->used_sz], 10);
	return( 0 );
}


int
cf_buf_builder_chomp(cf_buf_builder *bb)
{
	if (bb->used_sz > 0)
		bb->used_sz--;
	return(0);
}

char *
cf_buf_builder_strdup(cf_buf_builder *bb)
{
	if (bb->used_sz == 0)	return(0);
	char *r = malloc(bb->used_sz+1);
	if (!r)	return(0);
	memcpy(r, bb->buf, bb->used_sz);
	r[bb->used_sz] = 0;
	return(r);
}

	
cf_buf_builder *
cf_buf_builder_create()
{
	cf_buf_builder *bb = malloc(1024);
	if (!bb)	return(0);
	bb->alloc_sz = 1024 - sizeof(cf_buf_builder);
	bb->used_sz = 0;
	return(bb);
}

void
cf_buf_builder_free(cf_buf_builder *bb)
{
	free(bb);
}
