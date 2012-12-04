#pragma once

#include "as_map.h"

/******************************************************************************
 * FUNCTION DECLARATIONS
 ******************************************************************************/
 
/**
 * Create a hashmap backed map
 * @param capacity of the table
 * @return a new as_map
 */
as_map * as_hashmap_new(uint32_t);
