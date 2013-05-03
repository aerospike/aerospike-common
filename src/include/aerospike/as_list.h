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
#include <inttypes.h>

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>
#include <aerospike/as_string.h>
#include <aerospike/as_integer.h>
#include <aerospike/as_iterator.h>

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_list_s as_list;
typedef struct as_list_hooks_s as_list_hooks;

typedef bool (* as_list_foreach_callback) (as_val *, void *);

struct as_list_hooks_s {
    void        (* destroy)(as_list *);

    uint32_t    (* hash)(const as_list *);
    
    uint32_t    (* size)(const as_list *);
    int         (* append)(as_list *, as_val *);
    int         (* prepend)(as_list *, as_val *);
    as_val *    (* get)(const as_list *, const uint32_t);
    int         (* set)(as_list *, const uint32_t, as_val *);
    as_val *    (* head)(const as_list *);
    as_list *   (* tail)(const as_list *);
    as_list *   (* drop)(const as_list *l, uint32_t n); // create duplicate list from pos n
    as_list *   (* take)(const as_list *l, uint32_t n); // create dup from head, n len

    bool        (* foreach)(const as_list *, void *, as_list_foreach_callback);

    as_iterator * (* iterator_init)(const as_list *, as_iterator *);
    as_iterator * (* iterator_new)(const as_list *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_list *     as_list_init(as_list *, void *source, const as_list_hooks *);
as_list *     as_list_new(void *source, const as_list_hooks *);

void           as_list_destroy(as_list *);
void           as_list_val_destroy(as_val *v);

uint32_t as_list_val_hash(const as_val * v);
char * as_list_val_tostring(const as_val * v);

void * as_list_source(const as_list *);


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline uint32_t as_list_size(as_list * l) {
    return as_util_hook(size, 0, l);
}

inline int as_list_append(as_list * l, as_val * v) {
    return as_util_hook(append, 1, l, v);
}

inline int as_list_prepend(as_list * l, as_val * v) {
    return as_util_hook(prepend, 1, l, v);
}

inline as_val * as_list_get(const as_list * l, const uint32_t i) {
    return as_util_hook(get, NULL, l, i);
}

inline int as_list_set(as_list * l, const uint32_t i, as_val * v) {
    return as_util_hook(set, 1, l, i, v);
}

inline as_val * as_list_head(const as_list * l) {
    return as_util_hook(head, NULL, l);
}

inline as_list * as_list_tail(const as_list * l) {
    return as_util_hook(tail, NULL, l);
}

inline as_list * as_list_drop(const as_list * l, uint32_t n) {
    return as_util_hook(drop, NULL, l, n);
}

inline as_list * as_list_take(const as_list * l, uint32_t n) {
    return as_util_hook(take, NULL, l, n);
}

inline void as_list_foreach(const as_list * l, void * context, bool (* foreach)(as_val *, void *)) {
    as_util_hook(foreach, false, l, context, foreach);
}

inline as_iterator * as_list_iterator_init(as_iterator *i, const as_list * l) {
    return as_util_hook(iterator_init, NULL, l, i);
}

inline as_iterator * as_list_iterator_new(const as_list * l) {
    return as_util_hook(iterator_new, NULL, l);
}

inline uint32_t as_list_hash(const as_list * l) {
    return as_util_hook(hash, 0, l);
}

inline as_val * as_list_toval(as_list * l) {
    return (as_val *) l;
}

inline as_list * as_list_fromval(as_val * v) {
    return as_util_fromval(v, AS_LIST, as_list);
}


/*
** HELPERS
*/

static inline int as_list_add_string(as_list * l, const char * s) {
    return as_list_append(l, (as_val *) as_string_new(strdup(s), true));
}

static inline int as_list_add_integer(as_list * l, int64_t i) {
    return as_list_append(l, (as_val *) as_integer_new(i));
}

static inline int as_list_add_list(as_list * l, as_list * l2) {
    return as_list_append(l, (as_val *) l2);
}

struct as_map_s;

static inline int as_list_add_map(as_list * l, struct as_map_s * m) {
    return as_list_append(l, (as_val *) m);
}


