#include "as_serializer.h"

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_serializer_init(as_serializer *, const void *, const as_serializer_hooks *);
extern inline int as_serializer_destroy(as_serializer *);

extern inline as_serializer * as_serializer_new(const void *, const as_serializer_hooks *);
extern inline int as_serializer_free(as_serializer *);

extern inline void * as_serializer_source(as_serializer *);

extern inline int as_serializer_serialize(as_serializer *, as_val *, as_buffer *);
extern inline int as_serializer_deserialize(as_serializer *, as_buffer *, as_val **);
