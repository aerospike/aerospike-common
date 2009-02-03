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

#include <arpa/inet.h> /// htonl, htons

#include "cf.h"

// Define this if you want extra sanity checks enabled
// #define CHECK 1

int 
msg_create(msg **m_r, msg_type type, const msg_template *mt, size_t mt_sz)
{
	// Figure out how many bytes you need
	int mt_rows = mt_sz / sizeof(msg_template);
	if (mt_rows <= 0)
		cf_assert(0, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_CRITICAL, "msg create: invalid parameter");
	unsigned int max_id = 0;
	for (int i=0;i<mt_rows;i++) {
		if (mt[i].id >= max_id) {
			max_id = mt[i].id;
		}
	}
	max_id++;
	
	// DEBUG - can tell if it's so sparse that we're wasting lots of memory
	if (max_id > mt_rows * 2) {
		// It would be nice if there was a human readable string for debugging
		// in the message descriptor
		D("msg_create: found sparse message, %d ids, only %d rows consider recoding",max_id,mt_rows);
	}
	
	
	// allocate memory (if necessary)
	size_t m_sz = sizeof(msg_field) * max_id;
	msg *m;
	size_t a_sz = sizeof(msg) + m_sz;
	a_sz = (a_sz / 512) + 512;
	m = cf_rc_alloc(a_sz);
	cf_assert(m, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_CRITICAL, "malloc");
	m->len = max_id;
	m->bytes_used = sizeof(msg) + m_sz; 
	m->bytes_alloc = a_sz;
	m->type = type;
	m->mt = mt;
	
	// debug - not strictly necessary, but saves the user if they
	// have an invalid field
	for (int i=0;i<max_id;i++)
		m->f[i].is_valid = false;
	
	// fill in the fields - rather minimalistic, save the dcache
	for (int i=0;i<mt_rows;i++) {
		msg_field *f = &(m->f[ mt[i].id ] );
		f->id = mt[i].id;
		f->type = mt[i].type;
		f->rc_free = f->free = 0;
		f->is_set = false;
		f->is_valid = true;
	}
	
	*m_r = m;
	
	return(0);
}

//
// Increment the reference count of the message.
// Everyone must call destroy in the end, and that must all be matched up

void
msg_incr_ref(msg *m)
{
	cf_rc_reserve(m);
}

void
msg_decr_ref(msg *m)
{
	cf_rc_release(m);
}



// THE MEAT!
// YOU'VE FOUND THE MEAT!
//
// Here's where the wire protocol is defined. All the rest is just copying
// stupid buffers around.
//
// Currently, there aren't a lot of alignment checks here. We assume we're
// running on an intel architecture where the chip does the fastest possible
// thing for us.
//
// Should probably have some kind of version field, because there are better
// ways of doing this on a number of levels
//
// Current protocol:
// uint32_t size-in-bytes (not including this header, network byte order)
// uint16_t type (still included in the header)
//      2 byte - field id
// 		1 byte - field type
//      3 bytes - field size
//      [x] - field
//      (6 + field sz)

// htonll - an 8 byte swap
//   swaps in flight between two buffers
//   ASSUMES htonl(1) != 1 !!! FIXME!
//     (should be ';' if htonl(1) == 1) ;-)
//   ripe for assembly language here!!! this is a little freakin GNARLY!!!



// msg_parse - parse a buffer into a message, which thus can be accessed
int 
msg_parse(msg *m, const byte *buf, const size_t buflen, bool copy)
{
	if (buflen < 6) {
		D("msg_parse: but not enough data! will get called again len %d need 6.",buflen);
		return(-2);
	}
	uint32_t len = *(uint32_t *) buf;
	len = ntohl(len) ;
	if (buflen < len + 6) {
		D("msg_parse: but not enough data! will get called again. len %d need %d",buflen, (len + 6));
		return(-2);
	}
	buf += 4;
	
	uint16_t type = *(uint16_t *) buf;
	type = ntohs(type);
	if (m->type != type) {
		D("msg_parse: trying to parse incoming type %d into msg type %d, bad bad",type, m->type);
		return(-1);
	}
	buf += 2;

	const byte *eob = buf + len;
	
	while (buf < eob) {
		
		// Grab the ID
		uint32_t id = (buf[0] << 8) | buf[1];
		buf += 2;
		
		// find the field in the message
		msg_field *mf;
		if (id >= m->len) {
			D(" received message with id greater than current definition, kind of OK, ignoring field");
			mf = 0;
		}
		else {
			mf = &(m->f[id]);
			if (! mf->is_valid ) {
				D(" received message with id no longer valid, kind of OK, ignoring field");
				mf = 0;
			}
		}
		
		msg_field_type ft = (msg_field_type) *buf;
		uint32_t flen = (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
		buf += 4;
		
		if (mf && (ft != mf->type)) {
			D(" received message with incorrect field type from definition, kind of OK, ignoring field");
			mf = 0;
		}
		
		if (mf) {
		
			switch (mf->type) {
				case M_FT_INT32:
					mf->u.i32 = htonl( *(uint32_t *) buf ); // problem is htonl really only works on unsigned?
					break;
				case M_FT_UINT32:
					mf->u.ui32 = htonl( *(uint32_t *) buf );
					break;
				case M_FT_INT64:
					mf->u.i64 = __bswap_64( *(uint64_t *) buf);
					// htonll_p( &(mf->u.i64) , buf);
					break;
				case M_FT_UINT64:
					mf->u.ui64 = __bswap_64( *(uint64_t *) buf);
					// htonll_p( &(mf->u.i64) , buf);
					break;
				case M_FT_STR:
				case M_FT_BUF:
					mf->field_len = flen;
					if (copy) {
						if (m->bytes_alloc - m->bytes_used >= flen) {
							mf->u.buf = ((byte *)m) + m->bytes_used;
							m->bytes_used += flen;
							mf->free = mf->rc_free = 0;
						}
						else {
							// TODO: add assert
							mf->u.buf = malloc(flen);
							mf->free = mf->u.buf;
							mf->rc_free = 0;
						}
						memcpy(mf->u.buf, buf, flen);
					}
					else {
						mf->u.buf = (byte *) buf; // compiler whinges here correctly, I bless this cast
						mf->rc_free = mf->free = 0;						
					}
					break;
				case M_FT_ARRAY:
				case M_FT_MESSAGE:
				default:
					D("msg_parse: field type not supported, but skipping over anyway: %d",mf->type);
			}
			mf->is_set = true;
		}
		buf += flen;
	};
	
	return(0);
}

//
int
msg_get_initial(size_t *size_r, msg_type *type_r, const byte *buf, const size_t buflen)
{
	// enuf bytes to tell yet?
	if (buflen < 6)
		return(-2);
	
	// grab from the buf, nevermind alignment (see note)
	uint32_t size = * (uint32_t *) buf;
	// !paws
	size = ntohl(size);
	// size does not include this header
	size += 6;
	// bob's you're uncle
	*size_r = size;
	
	buf += 4;
	uint16_t type = * (uint16_t *) buf;
	type = ntohs(type);
	*type_r = type;
	
	return( 0 );

}	


static inline size_t
msg_get_wire_field_size(const msg_field *mf) {
	
	switch (mf->type) {
		case M_FT_INT32:
		case M_FT_UINT32:
			return(4);
		case M_FT_INT64:
		case M_FT_UINT64:
			return(8);
		case M_FT_STR:
		case M_FT_BUF:
			if (mf->field_len >= ( 1 << 24 ))
				D("field length %d too long, not yet supported", mf->field_len);
			return(mf->field_len);
		case M_FT_ARRAY:
		case M_FT_MESSAGE:
		default:
			D("field type not supported, internal error: %d",mf->type);
	}
	return(0);
}

// returns the number of bytes written

static inline size_t
msg_stamp_field(byte *buf, const msg_field *mf)
{
	// Stamp the ID
	if (mf->id >= 1 << 16) {
		D("msg_stamp_field: ID too large!");
		return(0);
	}
	buf[0] = (mf->id >> 8) & 0xff;
	buf[1] = mf->id & 0xff;
	buf += 2;

	// stamp the type
	*buf++ = (msg_field_type) mf->type;
	
	// Stamp the field itself (forward over the length, we'll patch that later
	size_t flen;
	buf += 3;
	switch(mf->type) {
		case M_FT_INT32:
			flen = 4;
			int32_t *b_i32 = (int32_t *)buf;
			*b_i32 = htonl(mf->u.i32);
			break;
			
		case M_FT_UINT32:
			flen = 4;
			uint32_t *b_ui32 = (uint32_t *)buf;
			*b_ui32 = htonl(mf->u.ui32);
			break;
			
		case M_FT_INT64:
			flen = 8;
			int64_t *b_i64 = (int64_t *)buf;
			*b_i64 = __bswap_64(mf->u.i64);
//			htonll_p(buf, (&mf->u.i64) );
			break;

		case M_FT_UINT64:
			flen = 8;
			uint64_t *b_ui64 = (uint64_t *)buf;
			*b_ui64 = __bswap_64(mf->u.ui64);
//			htonll_p(buf, (&mf->u.ui64) );
			break;
			
		case M_FT_STR:
		case M_FT_BUF:
			flen = mf->field_len;
			memcpy(buf, mf->u.buf, flen); 
			break;
			
		case M_FT_ARRAY:
		case M_FT_MESSAGE:
		default:
			D("field type not supported, internal error: %d",mf->type);
			return(0);
	}
	
	// Now, patch the length back in
	buf[-3] = (flen >> 16) & 0xff;
	buf[-2] = (flen >> 8) & 0xff;
	buf[-1] = flen & 0xff;

	return(6 + flen);
}


// msg_tobuf - parse a message out into a buffer.
//    interesting point: for a sparse buffer, it's better to rip through
//    	the msg description and deference to the table, but for non-sparse
//    	it's kinder to the cache lines to just rip through the actual table
//    	we assume non-sparse at the moment

int 
msg_fillbuf(const msg *m, byte *buf, size_t *buflen)
{
	// debug!
	memset(buf, 0xff, *buflen);
	
	// Figure out the size
	size_t	sz = 6;
	
	for (int i=0;i<m->len;i++) {
		const msg_field *mf = &m->f[i];
		if (mf->is_valid && mf->is_set) {
			sz += 6 + msg_get_wire_field_size(mf);
		}
	}
	
	// validate the size
	if (sz > *buflen) {
		D("msg_fillbuf: passed in size too small");
		return(-2);
	}
	*buflen = sz;
	
	// stamp the size in the buf
	(* (uint32_t *) buf) = htonl(sz - 6);
	buf += 4;
	// stamp the type
	(* (uint16_t *) buf) = htons(m->type);
	buf += 2;
	
	// copy the fields
	for (int i=0;i<m->len;i++) {
		const msg_field *mf = &m->f[i];
		if (mf->is_valid && mf->is_set) {
			buf += msg_stamp_field(buf, mf);
		}		
	}
	
	return(0);
}

//
// Purpose of msg_reset is to reset its internal state and make it ready for more reading or parsing

void
msg_reset(msg *m)
{
	m->bytes_used = (m->len * sizeof(msg_field)) + sizeof(msg);
	for (int i=0 ; i < m->len ; i++) {
		if (m->f[i].is_valid) {
			if (m->f[i].is_set == true) {
//				D("msg_reset: freeing %p rcfree %p",m->f[i].free,m->f[i].rc_free);
				if (m->f[i].free) free(m->f[i].free);
				if (m->f[i].rc_free) cf_rc_releaseandfree(m->f[i].rc_free);
				m->f[i].is_set = false;
			}
		}
	}
	
}

// Getters and setters
int 
msg_get_uint32(const msg *m, int field_id, uint32_t *r)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",field_id);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_UINT32);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg %p: attempt to retrieve unset field %d",m,field_id);
//		msg_dump(m);
		*r = 0;
		return(-2);
	}

	*r = m->f[field_id].u.ui32;
	
	return(0);
}

int msg_get_int32(const msg *m, int field_id, int32_t *r)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",field_id);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_INT32 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_INT32);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg: attempt to retrieve unset field %d",field_id);
//		msg_dump(m);
		*r = 0;
		return(-2);
	}
	
	*r = m->f[field_id].u.i32;
	
	return(0);
}

int msg_get_uint64(const msg *m, int field_id, uint64_t *r)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",field_id);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT64 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_UINT64);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg %p: attempt to retrieve unset field %d",m,field_id);
//		msg_dump(m);
		*r = 0;
		return(-2);
	}
	
	*r = m->f[field_id].u.ui64;
	
	return(0);
}

int msg_get_int64(const msg *m, int field_id, int64_t *r)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",field_id);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_INT64 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_INT64);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg %p: attempt to retrieve unset field %d",m,field_id);
//		msg_dump(m);
		*r = 0;
		return(-2);
	}
	
	*r = m->f[field_id].u.i64;
	
	return(0);
}


int 
msg_get_str(const msg *m, int field_id, char **r, size_t *len, msg_get_type type)  // this length is strlen+1, the allocated size
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",field_id);
		*r = 0; *len = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_STR ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch getter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		*r = 0; *len = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg %p: attempt to retrieve unset field %d",m,field_id);
//		msg_dump(m);
		*r = 0; *len = 0;
		return(-2);
	}
	
	if (MSG_GET_DIRECT == type) {
		*r = m->f[field_id].u.str;
	}
	else if (MSG_GET_COPY_MALLOC == type) {
		*r = strdup( m->f[field_id].u.str );
		cf_assert(*r, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "malloc");
	} else if (MSG_GET_COPY_RC == type) {
		size_t sz = m->f[field_id].field_len + 1;
		*r = cf_rc_alloc(sz);
		memcpy(*r, m->f[field_id].u.str, sz);
	}
	else {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg_get_str: illegal type");
		return(-2);
	}

	if (len)
		*len = m->f[field_id].field_len;
	
	return(0);
}

int 
msg_get_buf(const msg *m, int field_id, byte **r, size_t *len, msg_get_type type)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",m,field_id);
		*r = 0; *len = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_BUF ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg %p: mismatch getter field type wants %d has %d",m,m->f[field_id].type, M_FT_BUF);
		*r = 0; *len = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg %p: attempt to retrieve unset field %d",m,field_id);
//		msg_dump(m);
		*r = 0; *len = 0;
		return(-2);
	}
	
	if (MSG_GET_DIRECT == type) {
		*r = m->f[field_id].u.buf;
	}
	else if (MSG_GET_COPY_MALLOC == type) {
		*r = malloc( m->f[field_id].field_len );
		cf_assert(*r, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "malloc");
		memcpy(*r, m->f[field_id].u.buf, m->f[field_id].field_len );
	}
	else if (MSG_GET_COPY_RC == type) {
		*r = cf_rc_alloc( m->f[field_id].field_len );
		cf_assert(*r, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "malloc");
		memcpy(*r, m->f[field_id].u.buf, m->f[field_id].field_len );
	} else {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg_get_str: illegal type");
		return(-2);
	}

	if (len)
		*len = m->f[field_id].field_len;
	
	return(0);
}

// A bytearray is always a reference counted object. Now, the cool thing, when
// someone wants to get one of these, is to already have the ref-counted object
// and simply increment and hand out the pointer.
// That would demand a different MSG_field_type that is a bytearray type,
// which is eminently reasonable

int 
msg_get_bytearray(const msg *m, int field_id, cf_bytearray **r)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field get",m,field_id);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_BUF ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg %p: mismatch getter field type wants %d has %d",m,m->f[field_id].type, M_FT_BUF);
		*r = 0;
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	if ( ! m->f[field_id].is_set ) {
//		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_NOTICE, "msg %p: attempt to retrieve unset field %d",m,field_id);
//		msg_dump(m);
		*r = 0;
		return(-2);
	}
	uint64_t field_len = m->f[field_id].field_len;
	*r = cf_rc_alloc( field_len + sizeof(cf_bytearray) );
	cf_assert(*r, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "rcalloc");
	(*r)->sz = field_len;
	memcpy((*r)->data, m->f[field_id].u.buf, field_len );
	
	return(0);
}


int 
msg_set_uint32(msg *m, int field_id, const uint32_t v)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT32 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_UINT32);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	m->f[field_id].rc_free = m->f[field_id].free = 0; // pains me. Should be unnecessary. valgrind complains.
#endif	

	m->f[field_id].is_set = true;
	m->f[field_id].u.ui32 = v;
	
	return(0);
}

int 
msg_set_int32(msg *m, int field_id, const int32_t v)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_INT32 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_INT32);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	m->f[field_id].rc_free = m->f[field_id].free = 0; // pains me. Should be unnecessary. valgrind complains.
#endif	

	m->f[field_id].is_set = true;
	m->f[field_id].u.i32 = v;
	
	return(0);
}

int 
msg_set_uint64(msg *m, int field_id, const uint64_t v)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_UINT64 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_UINT64);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	m->f[field_id].rc_free = m->f[field_id].free = 0; // pains me. Should be unnecessary. valgrind complains.
#endif

	m->f[field_id].is_set = true;
	m->f[field_id].u.ui64 = v;
	
	return(0);
}

int 
msg_set_int64(msg *m, int field_id, const int64_t v)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_INT64 ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_INT64);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	m->f[field_id].rc_free = m->f[field_id].free = 0; // pains me. Should be unnecessary. valgrind complains.
#endif	
	m->f[field_id].is_set = true;
	m->f[field_id].u.i64 = v;
	
	return(0);
}

int 
msg_set_str(msg *m, int field_id, const char *v, msg_set_type type)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}

	if ( m->f[field_id].type != M_FT_STR ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif

	msg_field *mf = &(m->f[field_id]);
	
	// free auld value if necessary
	if (mf->is_set) {
		if (mf->free) {	free(mf->free); mf->free = 0; }
		if (mf->rc_free) { cf_rc_releaseandfree(mf->rc_free); mf->rc_free = 0; }
	}
	
	mf->field_len = strlen(v)+1;
	
	if (MSG_SET_COPY == type) {
		size_t len = mf->field_len;
		// If we've got a little extra memory here already, use it
		if (m->bytes_alloc - m->bytes_used >= len) {
			mf->u.str = (char *) (((byte *)m) + m->bytes_used);
			m->bytes_used += len;
			mf->free = 0;
			memcpy(mf->u.str, v, len);
		}
		else {
			mf->u.str = strdup(v);
			cf_assert(mf->u.str, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_CRITICAL, "malloc");
			mf->free = mf->u.str;
			mf->rc_free = 0;
		}
	} else if (MSG_SET_HANDOFF_MALLOC == type) {
		mf->u.str = (char *)v; // compiler winges here, correctly, but I bless this -b
		mf->rc_free = 0; 
		mf->free = (char *)v;
	}
	else if (MSG_SET_HANDOFF_RC == type) {
		mf->u.str = (char *)v;
		mf->rc_free = (char *)v;
		mf->free = 0;
	}
		
	mf->is_set = true;
	
	return(0);
}

int msg_set_buf(msg *m, int field_id, const byte *v, size_t len, msg_set_type type)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_BUF ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	msg_field *mf = &(m->f[field_id]);
	
	// free auld value if necessary
	if (mf->is_set) {
		if (mf->free) {	free(mf->free); mf->free = 0; }
		if (mf->rc_free) { cf_rc_releaseandfree(mf->rc_free); mf->rc_free = 0; }
	}
	
	mf->field_len = len;
	
	if (MSG_SET_COPY == type) {
		// If we've got a little extra memory here already, use it
		if (m->bytes_alloc - m->bytes_used >= len) {
			mf->u.buf = ((byte *)m) + m->bytes_used;
			m->bytes_used += len;
			mf->rc_free = mf->free = 0;
		}
		// Or just malloc if we have to. Sad face.
		else {
			mf->u.buf = malloc(len);
			cf_assert(mf->u.buf, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_CRITICAL, "malloc");
			mf->free = mf->u.buf; // free on exit/reset
			mf->rc_free = 0;
		}

		memcpy(mf->u.buf, v, len);

	} else if (type == MSG_SET_HANDOFF_MALLOC) {
		mf->u.buf = (void *)v; // compiler winges here, correctly, but I bless this -b
		mf->free = (void *)v;
		mf->rc_free = 0;
	}
	else if (type == MSG_SET_HANDOFF_RC) {
		mf->u.buf = (void *)v; // compiler winges here, correctly, but I bless this -b
		mf->rc_free = (void *)v;
		mf->free = 0;
	}
		
	mf->is_set = true;
	
	return(0);
}

//
// There are some cases, like reusing a message, where you have a set field and
// you'd like to unset just that field
//

void msg_set_unset(msg *m, int field_id)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return; // not sure the meaning of ERROR - will it throw or not?
	}
#endif	
	msg_field *mf = &m->f[field_id];
	
	if (mf->is_set == false)	return;
	
	// for different field types, might have to free stuff that's already there?
	// or maybe it's an always thing?
	switch (mf->type) {
		case M_FT_BUF:
		case M_FT_STR:
			if (mf->free)	free(mf->free);
			if (mf->rc_free) cf_rc_releaseandfree(mf->rc_free);
			break;
		default:
			break;
	}
	
	mf->is_set = false;
}

int msg_set_bytearray(msg *m, int field_id, const cf_bytearray *v)
{
#ifdef CHECK	
	if (! m->f[field_id].is_valid) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: invalid id %d in field set",field_id);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
	
	if ( m->f[field_id].type != M_FT_BUF ) {
		cf_fault(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_ERROR, "msg: mismatch setter field type wants %d has %d",m->f[field_id].type, M_FT_STR);
		return(-1); // not sure the meaning of ERROR - will it throw or not?
	}
#endif	

	msg_field *mf = &(m->f[field_id]);
	
	// free auld value if necessary
	if (mf->is_set) {
		if (mf->free) {	free(mf->free); mf->free = 0; }
		if (mf->rc_free) { cf_rc_releaseandfree(mf->rc_free); mf->rc_free = 0; }
	}
	
	mf->field_len = v->sz;
	mf->u.buf = (void *) v->data; // yes, discarding const, and it's OK
	mf->rc_free = (void *) v;
	mf->free = 0;
			
	mf->is_set = true;
	
	return(0);
}




// very useful for test code! one can encode and decode messages and see if
// they're the same!

int 
msg_compare(const msg *m1, const msg *m2) {
	D("msg_compare: stub");
	return(-2);
}

// And, finally, the destruction of a message
void msg_destroy(msg *m) 
{
	if (cf_rc_release(m)) {
		for (int i=0;i<m->len;i++) {
			if (m->f[i].is_valid && m->f[i].is_set && m->f[i].free)
				free(m->f[i].free);
		}
			
		cf_rc_free(m);
	}
	return;
}

//
// Debug routine to dump all the information about a msg to stdout
//

void
msg_dump(const msg *m)
{
	printf("msg_dump: msg %p acount %d flen %d bytesused %d bytesallocd %d type %d  mt %p\n",
		m,(int)cf_rc_count((void *)m),m->len,m->bytes_used,m->bytes_alloc,m->type,m->mt);
	for (int i=0;i<m->len;i++) {
		printf("mf %02d: id %d isvalid %d iset %d\n",i,m->f[i].id,m->f[i].is_valid,m->f[i].is_set);
		if (m->f[i].is_valid && m->f[i].is_set) {
			switch(m->f[i].type) {
				case M_FT_INT32:
					printf("   type INT32 value %d\n",m->f[i].u.i32);
					break;
				case M_FT_UINT32:
					printf("   type UINT32 value %u\n",m->f[i].u.ui32);
					break;
				case M_FT_INT64:
					printf("   type INT64 value %"PRId64"\n",m->f[i].u.i64);
					break;
				case M_FT_UINT64:
					printf("   type UINT64 value %"PRIu64"\n",m->f[i].u.ui64);
					break;
				case M_FT_STR:
					printf("   type STR len %d value %s free %p\n",m->f[i].field_len,m->f[i].u.str,m->f[i].free);
					break;
				case M_FT_BUF:
					printf("   type BUF len %d free %p\n",m->f[i].field_len,m->f[i].free);
					int j = 0;
					for ( ;j<m->f[i].field_len ; j++) {
						printf("%02x ",m->f[i].u.buf[j]);
						if (j % 16 == 8) printf(" : ");
						if (j % 16 == 15) printf("\n");
					}
					if (j % 16 != 15) printf("\n");
					break;
				default:
					printf("   type %d unknown\n",m->f[i].type);
					break;
			}
		}
	}
}

