/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <stdbool.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>
#include <aerospike/as_iterator.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_map_s as_map;
typedef struct as_map_hooks_s as_map_hooks;

struct as_map_hooks_s {
    void         (* destroy)(as_map *);
    
    uint32_t    (* hash)(const as_map *);
    
    uint32_t    (* size)(const as_map *);
    int         (* set)(as_map *, const as_val *, const as_val *);
    as_val *    (* get)(const as_map *, const as_val *);
    int         (* clear)(as_map *);

    as_iterator * (*iterator_init)(const as_map *, as_iterator *);
    as_iterator * (*iterator_new)(const as_map *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
 
void          as_map_destroy(as_map *);     // use this
void		  as_map_val_destroy(as_val *); // internal

uint32_t	  as_map_val_hash(const as_val *);
char *		  as_map_val_tostring(const as_val *);

//******************************************************************************
//* INLINE FUNCTIONS
//******************************************************************************

inline uint32_t as_map_size(const as_map * m) {
    return as_util_hook(size, 0, m);
}

inline as_val * as_map_get(const as_map * m, const as_val * k) {
    return as_util_hook(get, NULL, m, k);
}

inline int as_map_set(as_map * m, const as_val * k, const as_val * v) {
    return as_util_hook(set, 1, m, k, v);
}

inline int as_map_clear(as_map * m) {
    return as_util_hook(clear, 1, m);
}

inline as_iterator * as_map_iterator_init(as_iterator *i, const as_map * m) {
    return as_util_hook(iterator_init, NULL, m, i);
}

inline as_iterator * as_map_iterator_new(const as_map * m) {
    return as_util_hook(iterator_new, NULL, m);
}

inline int as_map_hash(as_map * m) {
    return as_util_hook(hash, 0, m);
}

inline as_val * as_map_toval(const as_map * m) {
    return (as_val *) m;
}

inline as_map * as_map_fromval(const as_val * v) {
    return as_util_fromval(v, AS_MAP, as_map);
}
