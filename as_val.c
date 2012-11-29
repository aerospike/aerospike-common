#include "as_val.h"

uint32_t as_val_hash(as_val * v) {
    return (v != NULL && ((as_val *)v)->hash != NULL ? ((as_val *)v)->hash((as_val *)v) : 0);
}
    