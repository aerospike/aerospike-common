#include "as_module.h"

#include "internal.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline void * as_module_source(as_module *);
extern inline int as_module_init(as_module *);
extern inline int as_module_configure(as_module *, void *);

/**
 * Apply a function to a record, and return the results in an as_result
 */
extern inline int as_module_apply_record(as_module *, as_aerospike *, const char *, const char *, as_rec *, as_list *, as_result *);


/**
 * Apply a function to an input stream, and write the results to an output stream.
 */
extern inline int as_module_apply_stream(as_module *, as_aerospike *, const char *, const char *, as_stream *, as_list *, as_stream *);
