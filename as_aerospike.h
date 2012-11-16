#pragma once

#include "as_types.h"

typedef struct as_aerospike_s as_aerospike;
typedef struct as_aerospike_hooks_s as_aerospike_hooks;

struct as_aerospike_s {
    void * source;
    const as_aerospike_hooks * hooks;
};

struct as_aerospike_hooks_s {
    as_rec * (*get)(as_aerospike *, const char *, const char *, const char *);
    int (*put)(as_aerospike *, const char *, const char *, const char * key, as_map *);
    int (*update)(as_aerospike *, as_rec *);
    int (*remove)(as_aerospike *, as_rec *);
    int (*log)(as_aerospike *, const char *, int, int, const char *);
    int (*free)(as_aerospike *);
};

as_aerospike * as_aerospike_create(void *, const as_aerospike_hooks *);

int as_aerospike_free(as_aerospike *);

void * as_aerospike_source(as_aerospike *);

as_rec * as_aerospike_get(as_aerospike *, const char *, const char *, const char *);

int as_aerospike_put(as_aerospike *, const char *, const char *, const char * key, as_map *);

int as_aerospike_update(as_aerospike *, as_rec *);

int as_aerospike_remove(as_aerospike *, as_rec *);

int as_aerospike_log(as_aerospike *, const char *, int, int, const char *);
