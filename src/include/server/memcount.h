#include <stdlib.h>
#include <string.h>
#include "dynbuf.h"

static inline uint32_t ptr_hash_fn(void *key) {
    return((uint32_t)(*(uint64_t *)key));
}

/*
 * Type for selecting the field to be sorted on for memory count reporting.
 */
typedef enum sort_field_e {
    CF_ALLOC_SORT_NET_SZ,
    CF_ALLOC_SORT_DELTA_SZ,
    CF_ALLOC_SORT_NET_ALLOC_COUNT,
    CF_ALLOC_SORT_TOTAL_ALLOC_COUNT,
    CF_ALLOC_SORT_TIME_LAST_MODIFIED
} sort_field_t;

extern struct shash_s *mem_count_shash;
extern cf_atomic64 mem_count;
extern cf_atomic64 mem_count_mallocs;
extern cf_atomic64 mem_count_frees;
extern cf_atomic64 mem_count_callocs;
extern cf_atomic64 mem_count_reallocs;
extern cf_atomic64 mem_count_strdups;
extern cf_atomic64 mem_count_strndups;
extern cf_atomic64 mem_count_vallocs;

extern int mem_count_init();
extern void mem_count_stats();
extern int mem_count_alloc_info(char *file, int line, cf_dyn_buf *db);
extern void mem_count_report(sort_field_t sort_field, int top_n, cf_dyn_buf *db);
extern void mem_count_shutdown();
