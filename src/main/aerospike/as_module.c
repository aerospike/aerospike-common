/* 
 * Copyright 2008-2016 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <aerospike/as_module.h>
#include <aerospike/as_util.h>

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Get the source of the module.
 */
void * as_module_source(as_module * m) {
    return (m ? (void *) m->source : NULL);
}

/**
 * Module Destroyer.
 */
int as_module_destroy(as_module * m) {
    return as_util_hook(destroy, 1, m);
}

/**
 * Module Configurator.
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
 */
int as_module_update(as_module * m, as_module_event * e) {
    return as_util_hook(update, 1, m, e);
}

/**
 * Validates a UDF provided by a string.
 */
int as_module_validate(as_module * m, as_aerospike * as, const char * filename, const char * content, uint32_t size, as_module_error * error) {
    return as_util_hook(validate, 1, m, as, filename, content, size, error);
}

/**
 * Applies a UDF to a record with provided arguments.
 */
int as_module_apply_record(as_module * m, as_udf_context *ctx, const char * filename, const char * function, as_rec * r, as_list * args, as_result * res) {
    return as_util_hook(apply_record, 1, m, ctx, filename, function, r, args, res);
}

/**
 * Applies a UDF to a stream with provided arguments.
 */
int as_module_apply_stream(as_module * m, as_udf_context * ctx, const char * filename, const char * function, as_stream * istream, as_list * args, as_stream * ostream, as_result *res) {
    return as_util_hook(apply_stream, 1, m, ctx, filename, function, istream, args, ostream, res);
}
