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

// arraylist:
// Structure for arraylist -- note that all fields are in terms of elements,
// not in terms of bytes.  Total size (bytes) allocated for the element
// array is   sizeof( as_val * ) * capacity
// The Field "block_size" is misleading -- it is NOT bytes, but it is the unit
// of allocation to be used each time the list grows.  So, the block_size
// might be 8 (for example), which means we'll grow by
//   new_delta_bytes = multiplier * (sizeof( as_val *) * block_size)
// each time we want to grow the array (with realloc).
struct as_arraylist_source_s {
    struct as_val_s **   elements; // An allocated area for ptrs to list elements
    uint32_t    size;           // The current array size (element count)
    uint32_t    capacity;       // The total array size (max element count)
    uint32_t    block_size;     // The unit of allocation (e.g. 8 elements)
                                // Note that block_size == 0 means no more
                                // can be allocated
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
extern as_val * as_val_val_reserve(as_val *v);
extern uint32_t as_val_val_hash(const as_val *v);
extern char * as_val_val_tostring(const as_val *v);

/******************************************************************************
 * MACROS
 ******************************************************************************/
inline void as_val_init(as_val *v, as_val_t type, bool is_malloc) {
	v->type = type; 
    v->is_malloc = is_malloc; 
    v->count = 1;
}
 
#define as_val_type(__v)     (((as_val *)__v)->type)

#define as_val_destroy(__v) ( as_val_val_destroy((as_val *)__v) )

#define as_val_reserve(__v) ( as_val_val_reserve((as_val *)__v) )

#define as_val_hash(__v) ( as_val_val_hash((as_val *)__v) )

#define as_val_tostring(__v) ( as_val_val_tostring((as_val *)__v) )



/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/



