#include "as_aerospike.h"
#include <stdlib.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_aerospike_init(as_aerospike *, void *, const as_aerospike_hooks *);
extern inline int as_aerospike_destroy(as_aerospike *);

extern inline as_aerospike * as_aerospike_new(void *, const as_aerospike_hooks *);
extern inline int as_aerospike_free(as_aerospike *);

extern inline int as_aerospike_create(const as_aerospike *, const as_rec *);
extern inline int as_aerospike_update(const as_aerospike *, const as_rec *);
extern inline int as_aerospike_remove(const as_aerospike *, const as_rec *);
extern inline int as_aerospike_exists(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_log(const as_aerospike *, const char *, const int, const int, const char *);
