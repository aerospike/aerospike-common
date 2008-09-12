/*
 * msg.c
 * Brian Bulkowski
 * Citrusleaf, 2008
 * This is a generic binary format message parsing system
 * You create the definition of the message not by an IDL, but by 
 * a .H file. Eventually we're going to need to do a similar thing using java,
 * though, which would promote an IDL-style approach
 * All rights reserved
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "cf.h"

int 
msg_create(msg **m, const msg_desc *md, size_t md_sz, void *stack_buf, size_t stack_buf_sz)
{
	D("msg_create: stub");
	return(-1);
}

// msg_parse - parse a buffer into a message, which thus can be accessed
int 
msg_parse(msg *m, const void *buf, const size_t buflen)
{
	D("msg_parse: stub");
	return(-1);
}


// msg_tobuf - parse a message out into a buffer. Ret
int 
msg_fillbuf(const msg *m, void *buf, size_t *buflen)
{
	D("msg_fillbuf: stub");
	return(-1);
}
	

// Getters and setters
int 
msg_get_uint32(const msg *m, int field_id, uint32_t *r)
{
	D("msg_get_uint32: stub");
	return(-1);
}

int msg_get_int32(const msg *m, int field_id, int32_t *r)
{
	D("msg_get_int32: stub");
	return(-1);
}

int msg_get_uint64(const msg *m, int field_id, uint64_t *r)
{
	D("msg_get_uint64: stub");
	return(-1);
}

int msg_get_int64(const msg *m, int field_id, int64_t *r)
{
	D(" msg_get_int64: stub");
	return(-1);
}

int msg_get_str(const msg *m, int field_id, char **r, size_t *len)  // this length is strlen+1, the allocated size
{
	D("msg_get_str: stub");
	return(-1);
}

int msg_get_buf(const msg *m, int field_id, void **r, size_t *len)
{
	D("msg_get_buf: stub");
	return(-1);
}

int msg_set_uint32(const msg *m, int field_id, uint32_t v)
{
	D("msg_set_uint32: stub");
	return(-1);
}

int msg_set_int32(const msg *m, int field_id, int32_t v)
{
	D("msg_set_int32: stub");
	return(-1);
}

int msg_set_uint64(const msg *m, int field_id, uint64_t v)
{
	D("msg_set_uint64: stub");
	return(-1);
}

int msg_set_int64(const msg *m, int field_id, int64_t v)
{
	D("msg_set_int64: stub");
	return(-1);
}

int msg_set_str(const msg *m, int field_id, char *v)
{
	D("msg_set_str: stub");
	return(-1);
}

int msg_set_buf(const msg *m, int field_id, void *v, size_t len)
{
	D("msg_set_buf: stub");
	return(-1);
}


// And, finally, the destruction of a message
void msg_destroy(msg *m) 
{
	D("msg_destroy: stub");
	return;
}

