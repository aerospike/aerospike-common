#pragma once

#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#include <cf_atomic.h>
#include <cf_shash.h>

// forward decls
struct as_list_hooks_s;
struct as_map_hooks_s;
struct as_list_s;
struct as_stream_s;
struct as_serializer_hooks_s;

enum as_val_t {
    AS_UNKNOWN      = 0,
    AS_NIL          = 1,
    AS_BOOLEAN      = 2,
    AS_INTEGER      = 3,
    AS_STRING       = 4,
    AS_LIST         = 5,
    AS_MAP          = 6,
    AS_REC          = 7,
    AS_PAIR         = 8
} __attribute__((packed));

struct as_val_s {
    enum as_val_t 	type;                      // type identifier, specified in as_val_t
    bool        is_malloc;
    cf_atomic32 count;
};

// arraylist
struct as_arraylist_source_s {
    struct as_val_s **   elements;
    uint32_t    size;
    uint32_t    capacity;
    uint32_t    block_size;
};

// linkedlist
struct as_linkedlist_source_s {
    struct as_val_s *		head;
    struct as_list_s * 		tail; 	// must be a linkedlist
};

// list
struct as_list_s {
    struct as_val_s                _;
    union {
        struct as_arraylist_source_s    arraylist;
        struct as_linkedlist_source_s   linkedlist;  
        void                   *generic;    // can be used to extend, use wisely (or not at all)
    } u;
    const struct as_list_hooks_s *   hooks;
};

// hashmap

struct as_hashmap_source_s {
    shash * h;
};

// map

struct as_map_s {
    struct as_val_s          _;
    const struct as_map_hooks_s *    hooks;
    union { // put others in here as necessary
        struct as_hashmap_source_s   hashmap;
        void				*generic; // for future expansion
    } u;
};

// Serializer

struct as_serializer_s {
    bool            is_malloc;
    union {
        void        *generic;   // no serializers have 
    } u;
    const struct as_serializer_hooks_s * hooks;
};

//
// ITERATOR
//

struct as_hashmap_iterator_source_s {
    shash * h;
    shash_elem * curr;
    shash_elem * next;
    uint32_t pos;
    uint32_t size;
};

struct as_linkedlist_iterator_source_s {
    const struct as_linkedlist_source_s * list;
};

struct as_arraylist_iterator_source_s {
    const struct as_arraylist_source_s *  list;
    uint32_t        pos;
};

struct as_stream_iterator_source_s {
    const struct as_stream_s * stream;
    const struct as_val_s * next;
    bool done;
};

struct as_iterator_s {
	bool  is_malloc;
    union {
    	struct as_linkedlist_iterator_source_s     linkedlist;
        struct as_arraylist_iterator_source_s      arraylist;
    	struct as_hashmap_iterator_source_s        hashmap;
        struct as_stream_iterator_source_s         stream;
        void *generic;
    } u;
    const struct as_iterator_hooks_s * hooks;
};

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef enum as_val_t as_val_t;
typedef struct as_val_s as_val;



typedef void (*as_val_destroy_func) (as_val *v);
typedef uint32_t (*as_val_hash_func) (const as_val *v);
typedef char * (*as_val_tostring_func) (const as_val *v);

extern as_val_destroy_func g_as_val_destroy_func_table[];
extern as_val_hash_func g_as_val_hash_func_table[];
extern as_val_tostring_func g_as_val_tostring_func_table[];

extern void as_val_val_destroy(as_val *v);
extern int as_val_val_reserve(as_val *v);

/******************************************************************************
 * MACROS
 ******************************************************************************/
inline void as_val_init(as_val *v, as_val_t type, bool is_malloc) {
	v->type = type; 
    v->is_malloc = is_malloc; 
    v->count = 1;
}
 
#define as_val_type(__v)     (((as_val *)__v)->type)

#define as_val_destroy(__v) (__v ? as_val_val_destroy((as_val *)__v) : 0 )

#define as_val_reserve(__v) (__v ? as_val_val_reserve((as_val *)__v) : 0 )

#define as_val_hash(__v) \
    (__v ? (g_as_val_hash_func_table[ ((as_val *)__v)->type ] (__v) ) : 0 )

#define as_val_tostring(__v) \
    (__v ? ( g_as_val_tostring_func_table[ ((as_val *)__v)->type ] (__v) ) : NULL ) 

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/



