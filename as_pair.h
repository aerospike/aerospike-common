#pragma once

#include <sys/types.h>

#include "as_util.h"
#include "as_val.h"


/******************************************************************************
 * MACROS
 ******************************************************************************/

#define pair_new(a,b) \
    as_pair_new((as_val *) a, (as_val *) b)

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_pair_s as_pair;

struct as_pair_s {
    as_val _;
    as_val * _1;
    as_val * _2;
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_pair * as_pair_new(as_val *, as_val *);
as_pair * as_pair_init(as_pair *, as_val * _1, as_val * _2);

void       as_pair_destroy(as_pair *);
void       as_pair_val_destroy(as_val *);

uint32_t   as_pair_val_hash(const as_val *);
char *	   as_pair_val_tostring(const as_val *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

char *as_pair_tostring(const as_pair *);

inline uint32_t as_pair_hash(const as_pair *p) {
	return(0);
}
 
inline as_val * as_pair_1(as_pair * p) {
    return p->_1;
}

inline as_val * as_pair_2(as_pair * p) {
    return p->_2;
}

inline as_val * as_pair_toval(const as_pair * p) {
    return (as_val *)p;
}

inline as_pair * as_pair_fromval(const as_val * v) {
    return as_util_fromval(v, AS_PAIR, as_pair);
}