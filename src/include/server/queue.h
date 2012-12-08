#pragma once
#include "../cf_queue.h"

#ifdef CF_QUEUE_ALLOCSZ
#undef CF_QUEUE_ALLOCSZ
#define CF_QUEUE_ALLOCSZ (64 * 1024)
#endif

extern int cf_queue_reduce_reverse(cf_queue *q, cf_queue_reduce_fn cb, void *udata);

extern int cf_queue_delete_all(cf_queue *q);
