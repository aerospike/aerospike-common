#include "as_module.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define LOG(m) \
    // printf("%s:%d  -- %s\n",__FILE__,__LINE__, m);

/**
 * Module Initializer.
 * This sets up the module before use. This is called only once on startup.
 *
 * Proxies to `m->hooks->init(m, ...)`
 *
 * @param m the module being initialized.
 * @return 0 on success, otherwhise 1
 */
int as_module_init(as_module * m) {
    if ( !m ) return 1;
    if ( !m->hooks ) return 2;
    if ( !m->hooks->init ) return 3;
    return m->hooks->init(m);
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
int as_module_configure(as_module * m, void * c) {
    if ( !m ) return 1;
    if ( !m->hooks ) return 2;
    if ( !m->hooks->configure ) return 3;
    return m->hooks->configure(m,c);
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
    if ( !m ) return 1;
    if ( !m->hooks ) return 2;
    if ( !m->hooks->apply_record ) return 3;
    return m->hooks->apply_record(m, as, filename, function, r, args, res);

    // int n = 1000000;
    // time_t start, stop;
    // double total_time = 0;

    // for (int i = 0; i < n; i++) {
    //     time(&start);
    //     m->hooks->apply_record(m, as, filename, function, r, args, res);
    //     time(&stop);
    //     double call_time = difftime(stop,start);
    //     total_time += call_time;
    //     printf("%s.%s call #%d took %.8lf sec\n",filename,function,i,call_time);
    // }

    // printf("%s.%s was called %d times took %.8lf seconds\n",filename, function, n, total_time);
    // printf("%s.%s was about %.8lf call/sec\n", filename, function, (n/total_time));
    // printf("%s.%s was about %.8lf sec/call\n", filename, function, (total_time/n));
    // printf("\n");
    // fflush(NULL);
    // return 0;
}

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
int as_module_apply_stream(as_module * m, as_aerospike * as, const char * filename, const char * function, as_stream * s, as_list * args, as_result * res) {
    if ( !m ) return 1;
    if ( !m->hooks ) return 2;
    if ( !m->hooks->apply_stream ) return 3;

    return m->hooks->apply_stream(m, as, filename, function, s, args, res);
}
