#include "as_memtracker.h"
#include "as_util.h"
#include <stdarg.h>
#include <stdio.h>

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated memtracker
 */
as_memtracker * as_memtracker_init(as_memtracker * memtracker, void * source, const as_memtracker_hooks * hooks) {
    if ( memtracker == NULL ) return memtracker;
    memtracker->source = source;
    memtracker->hooks = hooks;
    return memtracker;
}

/**
 * Heap allocate and initialize a memtracker
 */
as_memtracker * as_memtracker_new(void * source, const as_memtracker_hooks * hooks) {
    as_memtracker * memtracker = (as_memtracker *) malloc(sizeof(as_memtracker));
    memtracker->source = source;
    memtracker->hooks = hooks;
    return memtracker;
}

/**
 * Release resources associated with the memtracker.
 * Calls memtracker->destroy. If success and if this is a heap allocated
 * memtracker, then it will be freed.
 */
int as_memtracker_destroy(as_memtracker * memtracker) {
    int rc = as_util_hook(destroy, 1, memtracker);
    if ( rc == 0 && memtracker->is_malloc ) {
        free(memtracker);
    }
    return rc;
}

/**
 * For most purposes, you should use the macros:
 *   - as_memtracker_reserve(memtracker, num_bytes)
 *   - as_memtracker_release(memtracker, num_bytes)
 */
int as_memtracker_reserve(const as_memtracker * memtracker, const uint32_t num_bytes) {
    return as_util_hook(reserve, false, memtracker, num_bytes);
}

int as_memtracker_release(const as_memtracker * memtracker, const uint32_t num_bytes) {
    return as_util_hook(release, false, memtracker, num_bytes);
}
