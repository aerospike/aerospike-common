/*
*
* msg.h
* An independant message parser.
* This forms a generic way of parsing messages.
*
* A 'msg_desc' is a message descriptor. This will commonly be a singleton created
* by a unit once. 
* A 'msg' is a parsed-up and easy to read version of the message.
*/

#pragma once

#include "cf.h"

// NOTE: These values are actually used on the wire right now!type

typedef enum msg_field_type_t {
	M_FT_INT32 = 1,
	M_FT_UINT32 = 2,
	M_FT_INT64 = 3,
	M_FT_UINT64 = 4,
	M_FT_STR = 5,
	M_FT_BUF = 6,
	M_FT_ARRAY = 7,
	M_FT_MESSAGE = 8,
} msg_field_type;

// this is somewhat of a helper, because 
// in reality we never look at this values - they're for the caller to use.
// it's only important that the maximum value is respected
typedef enum msg_type_t {
	M_TYPE_FABRIC = 0,   // fabric's internal msg
	M_TYPE_HEARTBEAT = 1,
	M_TYPE_PAXOS = 2, // paxos' msg
	M_TYPE_MIGRATE = 3,
	M_TYPE_TEST = 4,
	M_TYPE_MAX = 5  /* highest + 1 is correct */
} msg_type;


typedef struct msg_field_template_t {
	unsigned int		id;
	msg_field_type 			type;
} msg_field_template;

// TODO: consider that a msg_desc should have a human readable string
// Can play other interesting macro games to make sure we don't have 
// the problem of needing to add a length field to the message descriptions

typedef msg_field_template msg_template;

// This is a very simple linear system for representing a message
// Insert/read efficiency is paramount, but we think messages will tend
// to be compact without a lot of holes. If we expected sparse representation,
// we'd use a data structure better at sparse stuff

typedef struct msg_field_t {
	unsigned int 		id; // really probabaly don't need this in the represenation we have
	msg_field_type 	type; 		// don't actually need this - but leave for faster access
	int 		field_len;  // used only for str and buf
	bool		is_valid;   // DEBUG - helps return errors on invalid types
	bool		is_set;     // keep track of whether the field was ever set
	union {
		uint32_t	ui32;
		int32_t		i32;
		uint64_t	ui64;
		int64_t     i64;
		char 		*str;
		byte		*buf;
		struct msg *m; // expansion - allows recursion - but how to describe?
	} u;
	void 		*free;  // this is a pointer that must be freed on destruction,
						// where exactly it points is a slight mystery
	void 		*rc_free;
} msg_field;


typedef struct msg_t {
	int		 len; // number of elements in the field structure
	size_t   bytes_used;
	size_t	 bytes_alloc;
	bool     is_stack; // allocated on stack no need to free
	msg_type type;
	const msg_template    *mt;  // the message descriptor used to create this
	msg_field   f[];
} msg;


//
// msg_create - Initialize an empty message. You can pass in a stack buff,
// too. If everything fits, it stays. We use the msg_desc as a hint
// Slightly unusually, the 'md_sz' field is in bytes. This is a good shortcut
// to avoid terminator fields or anything similar
extern int msg_create(msg **m, msg_type type, const msg_template *mt, size_t mt_sz);

// msg_parse - parse a buffer into a message, which thus can be accessed
extern int msg_parse(msg *m, const byte *buf, const size_t buflen, bool copy);

// If you've received a little bit of a buffer, grab the size header and type
// return = -2 means "not enough to tell yet"
extern int msg_get_initial(size_t *size, msg_type *type, const byte *buf, const size_t buflen);

// msg_tobuf - parse a message out into a buffer. Ret
extern int msg_fillbuf(const msg *m, byte *buf, size_t *buflen);

// msg_reset - after a message has been parsed, and the information consumed,
// reset all the internal pointers for another parse
extern void msg_reset(msg *m);

// Messages are reference counted. If you need to take a reference,
// call this function. Everyone calls destroy.
extern void msg_incr_ref(msg *m);

// Getters and setters

// Note! get_str and get_buf
// should probably be more complicated. To wit: both a 'copy' and 'dup'
// interface (where the 'copy' interface would return the length regardless,
// thus is also a 'getlet' method

// Note about 'get_buf' and 'get_bytearray'. These both operate on 'buf' type
// fields. The 'buf' calls, however, will either consume your pointer (and not free it later, does this still make sense?)
// or will take a copy of the data.
// The cf_bytearray version of 'get' will malloc you up a new cf_bytearray that you can take
// away for yoursef. The set_bytearray will do the same thing, take your cf_bytearray and free
// it later when the message is destroyed.

extern int msg_get_uint32(const msg *m, int field_id, uint32_t *r);
extern int msg_get_int32(const msg *m, int field_id, int32_t *r);
extern int msg_get_uint64(const msg *m, int field_id, uint64_t *r);
extern int msg_get_int64(const msg *m, int field_id, int64_t *r);
extern int msg_get_str(const msg *m, int field_id, char **r, size_t *len, bool copy);  // this length is strlen+1, the allocated size
extern int msg_get_buf(const msg *m, int field_id, byte **r, size_t *len, bool copy);
extern int msg_get_bytearray(const msg *m, int field_id, cf_bytearray **r);

extern int msg_set_uint32(msg *m, int field_id, const uint32_t v);
extern int msg_set_int32(msg *m, int field_id, const int32_t v);
extern int msg_set_uint64(msg *m, int field_id, const uint64_t v);
extern int msg_set_int64(msg *m, int field_id, const int64_t v);
extern int msg_set_str(msg *m, int field_id, const char *v, bool copy);
extern int msg_set_buf(msg *m, int field_id, const byte *v, size_t len, bool copy);
extern int msg_set_bytearray(msg *m, int field_id, const cf_bytearray *ba);

// A little routine that's good for testing
extern int msg_compare(const msg *m1, const msg *m2);

// And, finally, the destruction of a message
extern void msg_destroy(msg *m);

// a debug funtion for finding out what's in a message
extern void msg_dump(const msg *m);


