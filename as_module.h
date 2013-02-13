#pragma once

#include <stdlib.h>

#include "as_util.h"
#include "as_aerospike.h"
#include "as_stream.h"
#include "as_result.h"
#include "as_types.h"


/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_module_s as_module;
typedef struct as_module_hooks_s as_module_hooks;

/**
 * Module Structure.
 * Contains pointer to module specific data and a pointer to the
 * hooks that interface with the module.
 *
 * @field source contains module specific data.
 * @field hooks contains functions that can be applied to the module.
 */
struct as_module_s {
    const void * source;
    const as_module_hooks * hooks;
};

/**
 * Module Interface 
 * Provide functions which interface with a module.
 */
struct as_module_hooks_s {
    int (*init)(as_module *);
    int (*configure)(as_module *, void *);
    int (*apply_record)(as_module *, as_aerospike *, const char *, const char *, as_rec *, as_list *, as_result *);
    int (*apply_stream)(as_module *, as_aerospike *, const char *, const char *, as_stream *, as_list *, as_stream *);
};


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline void * as_module_source(as_module * m) {
    return (m ? (void *) m->source : NULL);
}

/**
 * Module Initializer.
 * This sets up the module before use. This is called only once on startup.
 *
 * Proxies to `m->hooks->init(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
inline int as_module_init(as_module * m) {
    return as_util_hook(init, 1, m);
}

/**
 * Module Configurator. 
 * This configures and reconfigures the module. This can be called an
 * arbitrary number of times during the lifetime of the server.
 *
 * Proxies to `m->hooks->configure(m, ...)`
 *
 * @param m the module being configured.
 * @return 0 on success, otherwhise 1
 */
inline int as_module_configure(as_module * m, void * c) {
    return as_util_hook(configure, 1, m, c);
}

/**
 * Applies a record and arguments to the function specified by a fully-qualified name.
 *
 * Proxies to `m->hooks->apply_record(m, ...)`
 *
 * @param m module from which the fqn will be resolved.
 * @param f fully-qualified name of the function to invoke.
 * @param r record to apply to the function.
 * @param args list of arguments for the function represented as vals 
 * @param result pointer to a val that will be populated with the result.
 * @return 0 on success, otherwise 1
 */
inline int as_module_apply_record(as_module * m, as_aerospike * as, const char * filename, const char * function, as_rec * r, as_list * args, as_result * res) {
    return as_util_hook(apply_record, 1, m, as, filename, function, r, args, res);
}

/**
 * Applies function to a stream and set of arguments. Pushes the results into an output stream.
 *
 * Proxies to `m->hooks->apply_stream(m, ...)`
 *
 * @param m module from which the fqn will be resolved.
 * @param f fully-qualified name of the function to invoke.
 * @param istream pointer to a readable stream, that will provides values.
 * @param args list of arguments for the function represented as vals 
 * @param ostream pointer to a writable stream, that will be populated with results.
 * @return 0 on success, otherwise 1
 */
inline int as_module_apply_stream(as_module * m, as_aerospike * as, const char * filename, const char * function, as_stream * istream, as_list * args, as_stream * ostream) {
    return as_util_hook(apply_stream, 1, m, as, filename, function, istream, args, ostream);
}
