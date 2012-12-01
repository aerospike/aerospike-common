#include "as_module.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline void * as_module_source(as_module *);
inline int as_module_init(as_module *);
inline int as_module_configure(as_module *, void *);
inline int as_module_apply_record(as_module *, as_aerospike *, const char *, const char *, as_rec, as_list *, as_result *);
inline int as_module_apply_stream(as_module *, as_aerospike *, const char *, const char *, as_stream, as_list *, as_result *);