#pragma once

#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#include <cf_atomic.h>

#include <as_internal.h>

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



