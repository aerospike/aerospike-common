/*
 * A simple dynamic buffer implementation
 * Allows the first, simpler part of the buffer to be on the stack
 * which is usually all that's needed
 *
 * Use pattern is a little funky, because the macro defines the stack variable.
 * Thus:
 *   define_dyn_buf( mydynbuf );
 *   
 
 * And you can keep adding cool things to it
 * Copywrite 2009 Brian Bulkowski
 * All rights reserved
 */

#pragma once
 
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#include "cf.h"


typedef struct cf_dyn_buf_s {
	uint8_t		*buf;
	bool		is_stack;
	size_t		alloc_sz;
	size_t		used_sz;
} cf_dyn_buf;

#define cf_dyn_buf_define(__x)  uint8_t dyn_buf##__x[1024]; cf_dyn_buf __x = { dyn_buf##__x, true, 1024, 0 } 

extern int cf_dyn_buf_append_string(cf_dyn_buf *db, char *s);
extern int cf_dyn_buf_append_char(cf_dyn_buf *db, char c);
extern int cf_dyn_buf_append_buf(cf_dyn_buf *db, uint8_t *buf, size_t sz);
extern int cf_dyn_buf_append_int(cf_dyn_buf *db, int i);
extern int cf_dyn_buf_append_uint64_x(cf_dyn_buf *db, uint64_t i);  // HEX FORMAT!
extern int cf_dyn_buf_append_uint64(cf_dyn_buf *db, uint64_t i);
extern int cf_dyn_buf_append_uint32(cf_dyn_buf *db, uint32_t i);
extern int cf_dyn_buf_chomp(cf_dyn_buf *db);
extern char *cf_dyn_buf_strdup(cf_dyn_buf *db);
extern void cf_dyn_buf_free(cf_dyn_buf *db);
