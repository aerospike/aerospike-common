#pragma once

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
 * TYPES
 *****************************************************************************/

struct as_memtracker_hooks_s;
typedef struct as_memtracker_hooks_s as_memtracker_hooks;

struct as_memtracker_s;
typedef struct as_memtracker_s as_memtracker;

/**
 * The interface which all memtrackers should implement.
 */
struct as_memtracker_hooks_s {
    /**
     * The destroy should free resources associated with the memtracker's source.
     * The destroy should not free the memtracker itself.
     */
    int (* destroy)(as_memtracker *);

    int (* reserve)(const as_memtracker *, const uint32_t);
    int (* release)(const as_memtracker *, const uint32_t);
};

/**
 * Logger handle
 */
struct as_memtracker_s {
    bool                    is_malloc;
    void *                  source;
    const as_memtracker_hooks * hooks;
};

/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated memtracker
 */
as_memtracker * as_memtracker_init(as_memtracker * memtracker, void * source, const as_memtracker_hooks * hooks);

/**
 * Heap allocate and initialize a memtracker
 */
as_memtracker * as_memtracker_new(void * source, const as_memtracker_hooks * hooks);


inline void * as_memtracker_source(const as_memtracker * mt) {
    return (mt ? mt->source : NULL);
}

/**
 * Release resources associated with the memtracker.
 * Calls memtracker->destroy. If success and if this is a heap allocated
 * memtracker, then it will be freed.
 */
int as_memtracker_destroy(as_memtracker * memtracker);

/**
 * Reserve num_bytes bytes of memory
 */
int as_memtracker_reserve(const as_memtracker * memtracker, const uint32_t num_bytes);

/**
 * Release num_bytes bytes of memory
 */
int as_memtracker_release(const as_memtracker * memtracker, const uint32_t num_bytes);
