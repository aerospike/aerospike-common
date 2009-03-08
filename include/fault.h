/*
 *  Citrusleaf Foundation
 *  include/fault.h - fault management framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include <pthread.h>



/* SYNOPSIS
 * Fault scoping
 *
 * Faults are identified by a context, scope, and severity.  The context
 * describes where the fault occured; the scope, how limited the fault is;
 * and the severity describes the required action.
 */

/* cf_fault_context
 * NB: if you add or remove entries from this enum, you must also change
 * the corresponding strings structure in fault.c */
typedef enum {
	CF_MISC,
	CF_RCALLOC,
	CF_HASH,
	CF_RCHASH,
	CF_SHASH,
	CF_QUEUE,
	CF_REDBLACK
} cf_fault_context;

/* cf_fault_scope
 *     GLOBAL              fatal errors terminate all execution
 *     PROCESS             fatal errors terminate the process
 *     THREAD              fatal errors terminate the enclosing thread
 */
typedef enum { GLOBAL, PROCESS, THREAD } cf_fault_scope;

/* cf_fault_severity
 *     CRITICAL            fatal runtime panics
 *     WARNING             runtime errors
 *     INFO                informational or advisory messages
 *     DEBUG               debugging messages
 *     DETAIL              detailed debugging messages
 */
typedef enum { CRITICAL, WARNING, INFO, DEBUG, DETAIL } cf_fault_severity;


/* CF_FAULT_BACKTRACE_DEPTH
 * The maximum depth of a backtrace */
#define CF_FAULT_BACKTRACE_DEPTH 16


/* cf_fault_recovery_key, cf_fault_recovery_stack
 * */
typedef pthread_key_t cf_fault_recovery_key;

#define CF_FAULT_RECOVERY_STACK_MAXDEPTH 32
struct cf_fault_recovery_stack_t {
	int8_t sz;

	void (*fn[CF_FAULT_RECOVERY_STACK_MAXDEPTH])(void *);
	void *arg[CF_FAULT_RECOVERY_STACK_MAXDEPTH];
};
typedef struct cf_fault_recovery_stack_t cf_fault_recovery_stack;


/* Function declarations */
extern void cf_fault_event(const cf_fault_context, const cf_fault_scope, const cf_fault_severity, const char *fn, const int line, char *msg, ...);
#define cf_assert(a, context, scope, severity, msg, ...) ((void)((a) ? (void)0 : cf_fault_event((context), (scope), (severity), __func__, __LINE__, (msg), ##__VA_ARGS__)))
#define cf_crash(context, scope, msg, ...) (cf_fault_event((context), (scope), CRITICAL, __func__, __LINE__, (msg), ##__VA_ARGS__))
#define cf_warning(context, msg, ...) (cf_fault_event((context), THREAD, WARNING, NULL, 0, (msg), ##__VA_ARGS__))
#define cf_info(context, msg, ...) (cf_fault_event((context), THREAD, INFO, NULL, 0, (msg), ##__VA_ARGS__))
#define cf_debug(context, msg, ...) (cf_fault_event((context), THREAD, DEBUG, NULL, 0, (msg), ##__VA_ARGS__))
#define cf_detail(context, msg, ...) (cf_fault_event((context), THREAD, DETAIL, __func__, __LINE__, (msg), ##__VA_ARGS__))
extern char *cf_strerror(const int err);
extern int cf_fault_recovery_globalinit(cf_fault_recovery_key *rkey);
extern int cf_fault_recovery_localinit(cf_fault_recovery_key *rkey, cf_fault_recovery_stack *stack);
extern int cf_fault_recovery_push(cf_fault_recovery_key *rkey, void (*fn)(void *), void *arg);
extern int cf_fault_recovery_pop(cf_fault_recovery_key *rkey, int exec);
extern void cf_fault_recovery(cf_fault_recovery_key *rkey);
extern void cf_fault_init(const int argc, char **argv, const int excludec, char **exclude);
