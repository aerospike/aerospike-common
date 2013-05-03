/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <stdlib.h>

#include <aerospike/as_aerospike.h>
#include <aerospike/as_stream.h>
#include <aerospike/as_result.h>
#include <aerospike/as_types.h>
#include <aerospike/as_logger.h>
#include <aerospike/as_memtracker.h>

/*****************************************************************************
 * TYPES
 *****************************************************************************/

struct as_module_event_data_s;
typedef struct as_module_event_data_s as_module_event_data;

struct as_module_event_s;
typedef struct as_module_event_s as_module_event;

struct as_module_s;
typedef struct as_module_s as_module;

struct as_module_hooks_s;
typedef struct as_module_hooks_s as_module_hooks;

/**
 * Module events.
 *
 * as_module_event e;
 * e.type = AS_MODULE_CONFIGURE;
 * e.data.config = my_config;
 */

enum as_module_event_type_e {
    AS_MODULE_EVENT_CONFIGURE     = 0,
    AS_MODULE_EVENT_FILE_SCAN     = 1,
    AS_MODULE_EVENT_FILE_ADD      = 2,
    AS_MODULE_EVENT_FILE_REMOVE   = 3,
};

typedef enum as_module_event_type_e as_module_event_type;

struct as_module_event_data_s {
    void * config;
    const char * filename;
};

struct as_module_event_s {
    as_module_event_type     type;
    as_module_event_data     data;
};


/**
 * Module Interface 
 * Provide functions which interface with a module.
 */
struct as_module_hooks_s {

    /**
     * Free resources used by the module.
     */
    int (* destroy)(as_module *);

    /**
     * Dispatch an event to the module.
     */
    int (* update)(as_module *, as_module_event *);

    /**
     * Apply a functio to a record
     */
    int (* apply_record)(as_module *, as_aerospike *, const char *, const char *, as_rec *, as_list *, as_result *);

    /**
     * Apply a function to a stream.
     */
    int (* apply_stream)(as_module *, as_aerospike *, const char *, const char *, as_stream *, as_list *, as_stream *);
};

/**
 * Module Structure.
 * Contains pointer to module specific data and a pointer to the
 * hooks that interface with the module.
 *
 * @field source contains module specific data.
 * @field hooks contains functions that can be applied to the module.
 */

struct as_module_s {
    const void *            source;
    as_logger *             logger;
    as_memtracker *         memtracker;
    const as_module_hooks * hooks;
};


/*****************************************************************************
 * INLINE FUNCTIONS
 *****************************************************************************/

/**
 * Get the source of the module.
 *
 * @param m the module to get the source from.
 */
void * as_module_source(as_module * m);

/**
 * Get the logger for this module.
 */
as_logger * as_module_logger(as_module * m);

/**
 * Module Destroyer.
 * This frees up the resources used by the module.
 *
 * Proxies to `m->hooks->destroy(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
int as_module_destroy(as_module * m);

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
int as_module_configure(as_module * m, void * c);

/**
 * Update a Module.
 *
 * Proxies to `m->hooks->update(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
int as_module_update(as_module * m, as_module_event * e);

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
int as_module_apply_record(as_module * m, as_aerospike * as, const char * filename, const char * function, as_rec * r, as_list * args, as_result * res);

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
int as_module_apply_stream(as_module * m, as_aerospike * as, const char * filename, const char * function, as_stream * istream, as_list * args, as_stream * ostream);
