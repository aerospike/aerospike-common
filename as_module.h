#ifndef _AS_MODULE_H
#define _AS_MODULE_H

#include <stdlib.h>

#include "as_stream.h"
#include "as_result.h"
#include "as_types.h"

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
    int (*configure)(as_module *);
    int (*apply_record)(as_module *, const char *, as_rec *, as_list *, as_result *);
    int (*apply_stream)(as_module *, const char *, as_stream *, as_list *, as_result *);
};

/**
 * Module Initializer.
 * This sets up the module before use. This is called only once on startup.
 *
 * Proxies to `m->hooks->init(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
int as_module_init(as_module *);

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
int as_module_configure(as_module *);

/**
 * Applies a function to a record and set of arguments.
 *
 * Proxies to `m->hooks->apply_record(m, ...)`
 *
 * @param m module from which the fqn will be resolved.
 * @param fqn fully-qualified name of the function to invoke.
 * @param r record to apply to the function.
 * @param args list of arguments for the function represented as vals 
 * @param result pointer to a val that will be populated with the result.
 * @return 0 on success, otherwise 1
 */
int as_module_apply_record(as_module *, const char *, as_rec *, as_list *, as_result *);

/**
 * Applies function to a stream and set of arguments.
 *
 * Proxies to `m->hooks->apply_stream(m, ...)`
 *
 * @param m module from which the fqn will be resolved.
 * @param f fully-qualified name of the function to invoke.
 * @param s stream to apply to the function.
 * @param args list of arguments for the function represented as vals 
 * @param result pointer to a val that will be populated with the result.
 * @return 0 on success, otherwise 1
 */
int as_module_apply_stream(as_module *, const char *, as_stream *, as_list *, as_result *);

#endif // _AS_MODULE_H