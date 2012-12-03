#include "as_result.h"
#include <stdlib.h>

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_result_free(as_result *);
extern inline int as_result_init(as_result *, bool, as_val *);

extern inline int as_result_destroy(as_result *);

extern inline as_result * as_success(as_val *);
extern inline as_result * as_failure(as_val *);

extern inline int as_result_tosuccess(as_result *, as_val *);
extern inline int as_result_tofailure(as_result *, as_val *);
