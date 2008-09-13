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
msg_create(msg **m_r, const msg_desc *md, size_t md_sz, byte *stack_buf, size_t stack_buf_sz)
{
	// Figure out how many bytes you need
	int md_rows = md_sz / sizeof(msg_desc);
	int max_id = -1;
	for (int i=0;i<md_rows;i++) {
		if (md[i].id >= max_id) {
			max_id = md[i].id;
		}
	}

	// DEBUG - can tell if it's so sparse that we're wasting lots of memory
	if (max_id > md_rows * 2) {
		// It would be nice if there was a human readable string for debugging
		// in the message descriptor
		D("msg_create: found sparse message, %d ids, only %d rows consider recoding",max_id,md_rows);
	}
	
	// allocate memory (if necessary)
	size_t m_sz = sizeof(msg_field) * max_id;
	msg *m;
	if ((stack_buf == 0) || (m_sz > stack_buf_sz)) {
		m = malloc(sizeof(msg) + m_sz);
		cf_assert(m, CF_FAULT_SEVERITY_CRITICAL, CF_FAULT_SCOPE_THREAD, "malloc");
		m->len = max_id;
		m->bytes_used = m->bytes_alloc = sizeof(msg) + m_sz;
		m->is_stack = false;
	} else {
		m = (msg *) stack_buf;
		m->len = max_id;
		m->bytes_used = sizeof(msg) + m_sz;
		m->bytes_alloc = stack_buf_sz;
		m->is_stack = true;
	}
	
	m->md = md;
	
	// debug - not strictly necessary, but saves the user if they
	// have an invalid field
	for (int i=0;i<max_id;i++)
		m->f[i].is_valid = false;
	
	// fill in the fields - rather minimalistic, save the dcache
	for (int i=0;i<md_rows;i++) {
		msg_field *f = &(m->f[ md[i].id ] );
		f->id = i;
		f->type = md[i].type;
		f->is_set = false;
		f->is_valid = true;
	}
	
	*m_r = m;
	
	return(0);
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
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field get",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_UINT32);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( ! m->f[field_id].is_set ) {
		cf_fault_event(CF_FAULT_SEVERITY_NOTICE, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: attempt to retrieve unset field %d",field_id);
		return(-2);
	}
	
	*r = m->f[field_id].u.ui32;
	
	return(0);
}

int msg_get_int32(const msg *m, int field_id, int32_t *r)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field get",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_INT32);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( ! m->f[field_id].is_set ) {
		cf_fault_event(CF_FAULT_SEVERITY_NOTICE, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: attempt to retrieve unset field %d",field_id);
		return(-2);
	}
	
	*r = m->f[field_id].u.i32;
	
	return(0);
}

int msg_get_uint64(const msg *m, int field_id, uint64_t *r)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field get",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_UINT64);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( ! m->f[field_id].is_set ) {
		cf_fault_event(CF_FAULT_SEVERITY_NOTICE, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: attempt to retrieve unset field %d",field_id);
		return(-2);
	}
	
	*r = m->f[field_id].u.ui64;
	
	return(0);
}

int msg_get_int64(const msg *m, int field_id, int64_t *r)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field get",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_INT64);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( ! m->f[field_id].is_set ) {
		cf_fault_event(CF_FAULT_SEVERITY_NOTICE, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: attempt to retrieve unset field %d",field_id);
		return(-2);
	}
	
	*r = m->f[field_id].u.i64;
	
	return(0);
}


int 
msg_get_str(const msg *m, int field_id, char **r, size_t *len, bool copy)  // this length is strlen+1, the allocated size
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field get",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_STR ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( ! m->f[field_id].is_set ) {
		cf_fault_event(CF_FAULT_SEVERITY_NOTICE, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: attempt to retrieve unset field %d",field_id);
		return(-2);
	}
	
	if (copy) {
		*r = strdup( m->f[field_id].u.str );
		cf_assert(*r, CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, "malloc");
	}
	else
		*r = m->f[field_id].u.str;

	*len = m->f[field_id].field_len;
	
	return(0);
}

int 
msg_get_buf(const msg *m, int field_id, byte **r, size_t *len, bool copy)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field get",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_BUF ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_BUF);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( ! m->f[field_id].is_set ) {
		cf_fault_event(CF_FAULT_SEVERITY_NOTICE, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: attempt to retrieve unset field %d",field_id);
		return(-2);
	}
	
	if (copy) {
		*r = malloc( m->f[field_id].field_len );
		cf_assert(*r, CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, "malloc");
		memcpy(*r, m->f[field_id].u.buf, m->f[field_id].field_len );
	}
	else
		*r = m->f[field_id].u.buf;

	*len = m->f[field_id].field_len;
	
	return(0);
}

int 
msg_set_uint32(msg *m, int field_id, const uint32_t v)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_UINT32);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	m->f[field_id].is_set = true;
	m->f[field_id].u.ui32 = v;
	
	return(0);
}

int 
msg_set_int32(msg *m, int field_id, const int32_t v)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_INT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_INT32);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	m->f[field_id].is_set = true;
	m->f[field_id].u.i32 = v;
	
	return(0);
}

int 
msg_set_uint64(msg *m, int field_id, const uint64_t v)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT64 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_UINT64);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	m->f[field_id].is_set = true;
	m->f[field_id].u.ui64 = v;
	
	return(0);
}

int 
msg_set_int64(msg *m, int field_id, const int64_t v)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_INT64);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	m->f[field_id].is_set = true;
	m->f[field_id].u.i64 = v;
	
	return(0);
}

int 
msg_set_str(msg *m, int field_id, const char *v, bool copy)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_STR ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	msg_field *mf = &(m->f[field_id]);
	
	// free auld value if necessary
	if (mf->is_set && mf->is_copy) {
		free(mf->u.str);
		mf->u.str = 0;
	}
	
	mf->field_len = strlen(v)+1;
	
	if (copy) {
		size_t len = mf->field_len;
		// If we've got a little extra memory here already, use it
		if (m->bytes_alloc - m->bytes_used >= len) {
			mf->u.str = (char *) (((byte *)m) + m->bytes_used);
			m->bytes_alloc += len;
			mf->is_copy = false;
			memcpy(mf->u.str, v, len);
		}
		else {
			mf->u.str = strdup(v);
			cf_assert(mf->u.str, CF_FAULT_SEVERITY_CRITICAL, CF_FAULT_SCOPE_THREAD, "malloc");
			mf->is_copy = true;
		}
	} else {
		mf->u.str = (char *)v; // compiler winges here, correctly, but I bless this -b
		mf->is_copy = false;
	}
		
	mf->is_set = true;
	
	return(0);
}


int msg_set_buf(msg *m, int field_id, const byte *v, size_t len, bool copy)
{
	if (! m->f[field_id].is_valid) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_STR ) {
		cf_fault_event(CF_FAULT_SEVERITY_ERROR, CF_FAULT_SCOPE_THREAD, __func__, __LINE__, 
			"msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	msg_field *mf = &(m->f[field_id]);
	
	// free auld value if necessary
	if (mf->is_set && mf->is_copy) {
		free(mf->u.buf);
		mf->u.buf = 0;
	}
	
	mf->field_len = len;
	
	if (copy) {
		// If we've got a little extra memory here already, use it
		if (m->bytes_alloc - m->bytes_used >= len) {
			mf->u.buf = ((byte *)m) + m->bytes_used;
			m->bytes_alloc += len;
			mf->is_copy = false;
		}
		// Or just malloc if we have to. Sad face.
		else {
			mf->u.buf = malloc(len);
			cf_assert(mf->u.buf, CF_FAULT_SEVERITY_CRITICAL, CF_FAULT_SCOPE_THREAD, "malloc");
			mf->is_copy = true;
		}

		memcpy(mf->u.buf, v, len);

	} else {
		mf->u.str = (void *)v; // compiler winges here, correctly, but I bless this -b
		mf->is_copy = false;
	}
		
	mf->is_set = true;
	
	return(0);
}


// And, finally, the destruction of a message
void msg_destroy(msg *m) 
{
	for (int i=0;i<m->len;i++) {
		if (m->f[i].is_valid && m->f[i].is_set && m->f[i].is_copy)
			free(m->f[i].u.buf);
	}
		
	if (! m->is_stack)
		free(m);

	return;
}

