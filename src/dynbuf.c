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

int
cf_dyn_buf_append_buf(cf_dyn_buf *db, uint8_t *buf, size_t sz)
{
	// see if we need more space
	if ( db->alloc_sz - db->used_sz < sz ) {
		uint8_t	*_t;
		size_t new_sz = db->alloc_sz + sz;
		
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
	memcpy(&db->buf[db->used_sz], buf, sz);
	db->used_sz += sz;
	return(0);
}

int
cf_dyn_buf_append_string(cf_dyn_buf *db, char *s)
{
	size_t	len = strlen(s);
	return ( cf_dyn_buf_append_buf(db, (uint8_t *) s, len) );
}

int
cf_dyn_buf_append_char (cf_dyn_buf *db, char c)
{
	return( cf_dyn_buf_append_buf(db, (uint8_t *) &c, sizeof(char) ) );
}


void
cf_dyn_buf_free(cf_dyn_buf *db)
{
	if (db->is_stack == false)
		free(db->buf);
}
