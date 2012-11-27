#pragma once

#include "as_map.h"

/**
 * Create a hashmap backed map
 * @param initial size of the table
 * @param steps for each capacity increase
 * @param function for hashing values
 * @return a new as_map
 */
as_map * as_hashmap_new(uint32_t, uint32_t, as_val_hash_function);
