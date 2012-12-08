/*
 * cf_hooks.c
 * 
 * Allows callers to define their own mutex and (and soon, allocation)
 * functions if for some reason they don't like the standard ones.
 *
 * Copyright 2012, Citrusleaf inc.
 */
#include "client/cf_hooks.h"

cf_mutex_hooks *g_mutex_hooks = (void *)0;
