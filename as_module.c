#include "as_module.h"
#include "internal.h"
#include "as_util.h"

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Get the source of the module.
 *
 * @param m the module to get the source from.
 */
void * as_module_source(as_module * m) {
    return (m ? (void *) m->source : NULL);
}

/**
 * Get the logger for this module.
 */
as_logger * as_module_logger(as_module * m) {
    return (m ? (void *) m->logger : NULL);
}


/**
 * Module Destroyer.
 * This frees up the resources used by the module.
 *
 * Proxies to `m->hooks->destroy(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
int as_module_destroy(as_module * m) {
    return as_util_hook(destroy, 1, m);
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
int as_module_configure(as_module * m, void * config) {
    as_module_event e = {
        .type = AS_MODULE_EVENT_CONFIGURE,
        .data.config = config
    };
    return as_module_update(m, &e);
}

/**
 * Update a Module.
 *
 * Proxies to `m->hooks->update(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
int as_module_update(as_module * m, as_module_event * e) {
    return as_util_hook(update, 1, m, e);
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
int as_module_apply_record(as_module * m, as_aerospike * as, const char * filename, const char * function, as_rec * r, as_list * args, as_result * res) {
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
int as_module_apply_stream(as_module * m, as_aerospike * as, const char * filename, const char * function, as_stream * istream, as_list * args, as_stream * ostream) {
    return as_util_hook(apply_stream, 1, m, as, filename, function, istream, args, ostream);
}
