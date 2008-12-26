/*
 *  Citrusleaf Foundation
 *  src/sort.c - sorting functions
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"


/* cf_sort_firstk_partition
 * Partition an array around an index */
uint64_t
cf_sort_firstk_partition(uint64_t *v, int left, int right, int index)
{
	uint64_t pivot = v[index];
    int i, s;

    /* Set the pivot to the beginning */
    cf_swap64(v, index, right);

    s = left;
    for (i = left; i < right; i++) {
        /* Don't swap equal elements */
        if (v[i] > pivot) {
            cf_swap64(v, s, i);
            s++;
        }
    }
    cf_swap64(v, right, s);

	return(s);
}


/* Some macros for a simple stack */
#define STACK_LIMIT 16
#define pop(s,offset) ((s)[--(offset)])
#define push(s,offset,v) ((s)[(offset)++] = (v))


/* cf_sort_firstk
 * Sort the largest k values to the head of the supplied array using a
 * stack-based implementation of Hoare's modified quickselect algorithm */
void
cf_sort_firstk(uint64_t *v, size_t sz, int k)
{
	int index = 0, left, right;
    int stack[STACK_LIMIT], sp = 0;

	/* Push the left and right limits onto the stack */
    push(stack, sp, 0);
    push(stack, sp, sz - 1);

	while (0 != sp) {
		right = pop(stack, sp);
		left = pop(stack, sp);

		if (right > left) {
			index = cf_sort_firstk_partition(v, left, right, (left + right) >> 1);

			/* If we exceed the stack depth, return a partially sorted list */
			if (sp >= STACK_LIMIT - 2)
				return;

			if (index < k) {
				push(stack, sp, index + 1);
				push(stack, sp, right);
			}

			push(stack, sp, left);
			push(stack, sp, index - 1);
		}
	}

	return;
}
