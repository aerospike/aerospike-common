#include <stdlib.h>
#include <cf_alloc.h>

#include "as_val.h"
#include "as_nil.h"
#include "as_boolean.h"
#include "as_integer.h"
#include "as_string.h"
#include "as_list.h"
#include "as_map.h"
#include "as_rec.h"
#include "as_pair.h"

#include "internal.h"

extern inline void as_val_init(as_val *v, as_val_t type, bool is_rcalloc);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void as_val_destroy_null_func(as_val *v) { return; }
uint32_t as_val_hash_null_func(const as_val *v) { return(0); }
char * as_val_tostring_null_func(const as_val *v) { return(0); }

/******************************************************************************
 * VARIABLES
 ******************************************************************************/

as_val_destroy_func g_as_val_destroy_func_table[] = {
	as_val_destroy_null_func, /* UNKNOWN */
	as_val_destroy_null_func, /* AS_NIL */
	as_val_destroy_null_func, /* AS_BOOLEAN */
	as_integer_val_destroy, /* AS_INTEGER */
	as_string_val_destroy, /* AS_STRING */
	as_list_val_destroy, /* AS_LIST */
	as_map_val_destroy, /* AS_MAP */
	as_rec_val_destroy, /* AS_REC */
	as_pair_val_destroy /* AS_PAIR */
};

as_val_hash_func g_as_val_hash_func_table[] = {
	as_val_hash_null_func, /* UNKNOWN */
	as_val_hash_null_func, /* AS_NIL */
	as_boolean_val_hash, /* AS_BOOLEAN */
	as_integer_val_hash, /* AS_INTEGER */
	as_string_val_hash, /* AS_STRING */
	as_list_val_hash, /* AS_LIST */
	as_map_val_hash, /* AS_MAP */
	as_rec_val_hash, /* AS_REC */
	as_pair_val_hash /* AS_PAIR */
};

as_val_tostring_func g_as_val_tostring_func_table[] = {
	as_val_tostring_null_func, /* UNKNOWN */
	as_nil_val_tostring,      /* AS_NIL */
	as_boolean_val_tostring, /* AS_BOOLEAN */
	as_integer_val_tostring, /* AS_INTEGER */
	as_string_val_tostring, /* AS_STRING */
	as_list_val_tostring, /* AS_LIST */
	as_map_val_tostring, /* AS_MAP */
	as_rec_val_tostring, /* AS_REC */
	as_pair_val_tostring /* AS_PAIR */
};

int as_val_val_reserve(as_val *v) {
	int i = cf_atomic32_add(&(v->count),1);
	return( i );
}

void as_val_val_destroy(as_val *v) {
	if (v == 0)	return;
	// if we reach the last reference, call the destructor, and free
	if ( 0 == cf_atomic32_decr(&(v->count)) ) {
		g_as_val_destroy_func_table[ ((as_val *)v)->type ](v);		
		if (v->is_malloc) {
        	free(v);
        }
    }
	return;
}
