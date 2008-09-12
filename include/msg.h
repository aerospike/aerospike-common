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


typedef enum field_type_t {
	M_FT_INT32 = 1,
	M_FT_UINT32 = 2,
	M_FT_INT64 = 3,
	M_FT_UINT64 = 4,
	M_FT_STR = 5,
	M_FT_BUF = 6,
	M_FT_ARRAY = 7,
	M_FT_MESSAGE = 8,
} field_type;


typedef struct msg_field_desc_t {
	int		id;
	field_type ft;
} msg_field_desc;

typedef msg_field_desc msg_desc;

// This is a very simple linear system for representing a message


typedef struct msg_field_t {
	int 		id;
	field_type 	type;
	int 		field_len; // used only for str and buf
	union {
		uint32_t	ui32;
		int32_t		i32;
		uint64_t	ui64;
		int64_t     i64;
		char 		*str;
		void		*buf;
		struct msg *m; // expansion - allows recursion - but how to describe?
		
	} u;
} msg_field;


typedef struct msg_t {
	int		len;
	int     alloc_len;
	int     stack; // allocated on stack no need to free (can't find bool, as usual...)
	msg_desc    *md;  // the message descriptor used to create this
	msg_field   f[];
} msg;


//
// msg_create - Initialize an empty message. You can pass in a stack buff,
// too. If everything fits, it stays. We use the msg_desc as a hint
// Slightly unusually, the 'md_sz' field is in bytes. This is a good shortcut
// to avoid terminator fields or anything similar
extern int msg_create(msg **m, const msg_desc *md, size_t md_sz, void *stack_buf, size_t stack_buf_sz);

// msg_parse - parse a buffer into a message, which thus can be accessed
extern int msg_parse(msg *m, const void *buf, const size_t buflen);

// msg_tobuf - parse a message out into a buffer. Ret
extern int msg_fillbuf(const msg *m, void *buf, size_t *buflen);

// Getters and setters
extern int msg_get_uint32(const msg *m, int field_id, uint32_t *r);
extern int msg_get_int32(const msg *m, int field_id, int32_t *r);
extern int msg_get_uint64(const msg *m, int field_id, uint64_t *r);
extern int msg_get_int64(const msg *m, int field_id, int64_t *r);
extern int msg_get_str(const msg *m, int field_id, char **r, size_t *len);  // this length is strlen+1, the allocated size
extern int msg_get_buf(const msg *m, int field_id, void **r, size_t *len);

extern int msg_set_uint32(const msg *m, int field_id, uint32_t v);
extern int msg_set_int32(const msg *m, int field_id, int32_t v);
extern int msg_set_uint64(const msg *m, int field_id, uint64_t v);
extern int msg_set_int64(const msg *m, int field_id, int64_t v);
extern int msg_set_str(const msg *m, int field_id, char *v);
extern int msg_set_buf(const msg *m, int field_id, void *v, size_t len);

// And, finally, the destruction of a message
extern void msg_destroy(msg *m);


