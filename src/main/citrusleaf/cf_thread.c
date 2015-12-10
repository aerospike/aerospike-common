#if !defined ENHANCED_THREADS
#define ENHANCED_THREADS
#endif
#include "citrusleaf/cf_thread.h"

#include "aerospike/ck/ck_pr.h"

#include "citrusleaf/alloc.h"
#include "citrusleaf/cf_ll.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/syscall.h>
#include <sys/types.h>

#define HOLD_LIMIT 0x100
#define HOLD_MASK 0xff
#define HOLD_HASH(mutex) (( \
		((ptrdiff_t)mutex) ^ ((ptrdiff_t)mutex >> 8) ^ ((ptrdiff_t)mutex >> 16) \
	) & HOLD_MASK)

typedef struct {
	const char *file_name;
	pthread_mutex_t *mutex;
	int line_no;
	int pad[3];
} cf_mutex_info_t;

typedef struct {
	cf_mutex_info_t hold[HOLD_LIMIT];
	cf_mutex_info_t wait;
	cf_ll_element link;
	pid_t tid;
} cf_mutex_state_t;

typedef void *(*cf_start_routine_t)(void *);

typedef struct {
	cf_start_routine_t start_routine;
	void *data;
} proxy_data_t;

static __thread cf_mutex_state_t *tls_state = NULL;

static int init_count = 0;
static cf_ll info_list;

static void *start_routine_proxy(void *arg)
{
	unsigned char *memory = cf_malloc(sizeof (cf_mutex_state_t) + 0x40);
	assert(memory != NULL);

	unsigned char *aligned = (unsigned char *)(((ptrdiff_t)memory + 0x3f) & (ptrdiff_t)~0x3f);
	tls_state = (cf_mutex_state_t *)aligned;

	memset(tls_state, 0, sizeof (cf_mutex_state_t));
	tls_state->tid = syscall(SYS_gettid);

	if (ck_pr_faa_int(&init_count, 1) == 0) {
		cf_ll_init(&info_list, NULL, true);
	}

	cf_ll_append(&info_list, &tls_state->link);

	proxy_data_t *proxy_data = arg;
	void *res = proxy_data->start_routine(proxy_data->data);

	cf_ll_delete(&info_list, &tls_state->link);
	cf_free(memory);
	cf_free(arg);
	return res;
}

int cf_thread_create(pthread_t *thread, const pthread_attr_t *attr,
		cf_start_routine_t start_routine, void *arg)
{
	proxy_data_t *proxy_data = cf_malloc(sizeof (proxy_data_t));
	assert(proxy_data != NULL);

	proxy_data->start_routine = start_routine;
	proxy_data->data = arg;

	return pthread_create(thread, attr, start_routine_proxy, proxy_data);
}

static void set_waiting(cf_mutex_state_t *state, pthread_mutex_t *mutex,
		const char *file_name, int line_no)
{
	cf_mutex_info_t *info = &state->wait;
	assert(info->mutex == NULL);

	info->mutex = mutex;
	info->file_name = file_name;
	info->line_no = line_no;
}

static void clear_waiting(cf_mutex_state_t *state, pthread_mutex_t *mutex)
{
	cf_mutex_info_t *info = &state->wait;
	assert(info->mutex == mutex);

	info->mutex = NULL;
}

static void add_holding(cf_mutex_state_t *state, pthread_mutex_t *mutex,
		const char *file_name, int line_no)
{
	int base = HOLD_HASH(mutex);

	for (int i = 0; i < HOLD_LIMIT; ++i) {
		cf_mutex_info_t *info = &state->hold[(base + i) & HOLD_MASK];

		if (info->mutex == NULL) {
			info->mutex = mutex;
			info->file_name = file_name;
			info->line_no = line_no;
			return;
		}
	}

	assert(false);
}

static void remove_holding(cf_mutex_state_t *state, pthread_mutex_t *mutex)
{
	int base = HOLD_HASH(mutex);

	for (int i = 0; i < HOLD_LIMIT; ++i) {
		cf_mutex_info_t *info = &state->hold[(base + i) & HOLD_MASK];

		if (info->mutex == mutex) {
			info->mutex = NULL;
			return;
		}
	}

	assert(false);
}

int cf_mutex_lock_do(pthread_mutex_t *mutex, const char *file_name, int line_no)
{
	cf_mutex_state_t *state = tls_state;
	assert(state != NULL);

	set_waiting(state, mutex, file_name, line_no);
	int res = pthread_mutex_lock(mutex);
	clear_waiting(state, mutex);

	if (res == 0) {
		add_holding(state, mutex, file_name, line_no);
	}

	return res;
}

int cf_mutex_trylock_do(pthread_mutex_t *mutex, const char *file_name, int line_no)
{
	cf_mutex_state_t *state = tls_state;
	assert(state != NULL);

	int res = pthread_mutex_trylock(mutex);

	if (res == 0) {
		add_holding(state, mutex, file_name, line_no);
	}

	return res;
}

int cf_mutex_unlock_do(pthread_mutex_t *mutex)
{
	cf_mutex_state_t *state = tls_state;
	assert(state != NULL);

	int res = pthread_mutex_unlock(mutex);

	if (res == 0) {
		remove_holding(state, mutex);
	}

	return res;
}

static cf_mutex_state_t *link_to_state(cf_ll_element *link)
{
	unsigned char *state = (unsigned char *)link - offsetof (cf_mutex_state_t, link);
	return (cf_mutex_state_t *)state;
}

static int dump_reduce(cf_ll_element *link, void *arg)
{
	(void)arg;

	cf_mutex_state_t *state = link_to_state(link);
	fprintf(stderr, "--------------------- mutex state of thread %06d\n", state->tid);

	if (state->wait.mutex != NULL) {
		fprintf(stderr, "waiting for %p (%s:%d)\n", state->wait.mutex, state->wait.file_name,
				state->wait.line_no);
	}
	else {
		fprintf(stderr, "not waiting\n");
	}

	bool holding = false;

	for (int i = 0; i < HOLD_LIMIT; ++i) {
		if (state->hold[i].mutex != NULL) {
			fprintf(stderr, "holding %p (%s:%d)\n", state->hold[i].mutex, state->hold[i].file_name,
					state->hold[i].line_no);
			holding = true;
		}
	}

	if (!holding) {
		fprintf(stderr, "not holding\n");
	}

	return 0;
}

void cf_mutex_dump(void)
{
	if (ck_pr_load_int(&init_count) == 0) {
		fprintf(stderr, "no thread mutex states available\n");
		return;
	}

	cf_ll_reduce(&info_list, true, dump_reduce, NULL);
}
