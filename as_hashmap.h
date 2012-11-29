#pragma once

#include "as_map.h"

/**
 * Create a hashmap backed map
 * @param capacity of the table
 * @param hash function for keys
 * @return a new as_map
 */
as_map * as_hashmap_new(uint32_t);
