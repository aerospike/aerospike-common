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

#include <aerospike/as_util.h>
#include <aerospike/as_val.h>
#include <aerospike/as_bytes.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_rec_hooks_s;

/**
 * Record Structure
 * Contains a pointer to the source of the record and 
 * hooks that interface with the source.
 *
 * @field source contains record specific data.
 * @field hooks contains the record interface that works with the source.
 */
struct as_rec_s {
    as_val                          _;
    void *                          data;
    const struct as_rec_hooks_s *   hooks;
};

typedef struct as_rec_s as_rec;

/**
 * Record Interface.
 * Provided functions that interface with the records.
 */
struct as_rec_hooks_s {
    bool        (* destroy)(as_rec *);
    uint32_t    (* hashcode)(const as_rec *);

    as_val *    (* get)(const as_rec *, const char *);
    int         (* set)(const as_rec *, const char *, const as_val *);
    int         (* remove)(const as_rec *, const char *);
    uint32_t    (* ttl)(const as_rec *);
    uint16_t    (* gen)(const as_rec *);
    uint16_t    (* numbins)(const as_rec *);
    as_bytes *  (* digest)(const as_rec *);
};

typedef struct as_rec_hooks_s as_rec_hooks;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

as_rec *  as_rec_init(as_rec *, void *, const as_rec_hooks *);
as_rec *  as_rec_new(void *, const as_rec_hooks *);

void      as_rec_val_destroy(as_val *);
uint32_t  as_rec_val_hashcode(const as_val *v);
char *    as_rec_val_tostring(const as_val *v);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline void * as_rec_source(const as_rec * r) {
    return r ? r->data : NULL;
}

inline void as_rec_destroy(as_rec *r) {
    as_val_val_destroy((as_val *) r);
}



/**
 * Get a bin value by name.
 * The return value must be destroyed or passed to another function
 *
 * Proxies to `r->hooks->get(r, name, value)`
 *
 * @param r the as_rec to read the bin value from.
 * @param name the name of the bin.
 * @param a as_val containing the value in the bin.
 */
inline as_val * as_rec_get(const as_rec * r, const char * name) {
    return as_util_hook(get, NULL, r, name);
}

/**
 * Set the value of a bin.
 * This CONSUMES the reference on the value
 *
 * Proxies to `r->hooks->set(r, name, value)`
 *
 * @param r the as_rec to write the bin value to - CONSUMES REFERENCE
 * @param name the name of the bin.
 * @param value the value of the bin.
 */
inline int as_rec_set(const as_rec * r, const char * name, const as_val * value) {
    return as_util_hook(set, 1, r, name, value);
}

/**
 * Rmeove a bin from a record.
 *
 * Proxies to `r->hooks->set(r, name, value)`
 *
 * @param r the record to remove the bin from.
 * @param name the name of the bin to remove.
 */
inline int as_rec_remove(const as_rec * r, const char * name) {
    return as_util_hook(remove, 1, r, name);
}

inline uint32_t as_rec_ttl(const as_rec * r) {
    return as_util_hook(ttl, 0, r);
}

inline uint16_t as_rec_gen(const as_rec * r) {
    return as_util_hook(gen, 0, r);
}

inline uint16_t as_rec_numbins(const as_rec * r) {
    return as_util_hook(numbins, 0, r);
}

inline as_bytes * as_rec_digest(const as_rec * r) {
    return as_util_hook(digest, 0, r);
}



inline as_val * as_rec_toval(const as_rec * r) {
    return (as_val *) r;
}

inline as_rec * as_rec_fromval(const as_val * v) {
    return as_util_fromval(v, AS_REC, as_rec);
}
