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
 *
 * Examples:
 *    cf_info(CF_MISC, "important message: %s", my_msg);
 *    cf_crash(CF_MISC, THREAD, "doom!");
 *    cf_assert(my_test, CF_MISC, PROCESS, CRITICAL, "gloom!");
 */


/* cf_fault_context
 * NB: if you add or remove entries from this enum, you must also change
 * the corresponding strings structure in fault.c */
typedef enum {
	CF_MISC = 0,
	CF_RCALLOC = 1,
	CF_HASH = 2,
	CF_RCHASH = 3,
	CF_SHASH = 4,
	CF_QUEUE = 5,
	CF_MSG = 6,
	CF_RB = 7,
	CF_SOCKET = 8,
	CF_TIMER = 9,
	CF_LL = 10,
	AS_CFG = 11,
	AS_NAMESPACE = 12,
	AS_AS = 13,
	AS_BIN = 14,
	AS_RECORD = 15,
	AS_PROTO = 16,
	AS_PARTICLE = 17,
	AS_DEMARSHAL = 18,
	AS_WRITE = 19,
	AS_TSVC = 20,
	AS_TEST = 21,
	AS_NSUP = 22,
	AS_PROXY = 23,
	AS_HB = 24,
	AS_FABRIC = 25,
	AS_PARTITION = 26,
	AS_PAXOS = 27,
	AS_MIGRATE = 28,
	AS_INFO = 29,
	AS_STORAGE = 30,
	AS_DRV_MEM = 31,
	AS_DRV_FS = 32,
	CF_FAULT_CONTEXT_UNDEF = 33
} cf_fault_context;

/* cf_fault_scope
 *     GLOBAL              fatal errors terminate all execution
 *     PROCESS             fatal errors terminate the process
 *     THREAD              fatal errors terminate the enclosing thread
 */
typedef enum { CF_GLOBAL = 0, CF_PROCESS = 1, CF_THREAD = 2 } cf_fault_scope;

/* cf_fault_severity
 *     CRITICAL            fatal runtime panics
 *     WARNING             runtime errors
 *     INFO                informational or advisory messages
 *     DEBUG               debugging messages
 *     DETAIL              detailed debugging messages
 */
typedef enum { CF_CRITICAL = 0, CF_WARNING = 1, CF_INFO = 2, CF_DEBUG = 3, CF_DETAIL = 4, CF_FAULT_SEVERITY_UNDEF = 5 } cf_fault_severity;


/* cf_fault_sink
 * An endpoint (sink) for a flow of fault messages */
typedef struct cf_fault_sink {
	int fd;
	char *path;
	int limit[CF_FAULT_CONTEXT_UNDEF];
} cf_fault_sink;


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
extern int cf_fault_sink_addcontext(cf_fault_sink *s, char *context, char *severity);
extern cf_fault_sink *cf_fault_sink_add(char *path);
extern void cf_fault_event(const cf_fault_context, const cf_fault_scope, const cf_fault_severity severity, const char *fn, const int line, char *msg, ...);
#define cf_assert(a, context, scope, severity, __msg, ...) ((void)((a) ? (void)0 : cf_fault_event((context), (scope), (severity), __func__, __LINE__, (__msg), ##__VA_ARGS__)))
#define cf_crash(context, scope, __msg, ...) (cf_fault_event((context), (scope), CF_CRITICAL, __FILE__, __LINE__, (__msg), ##__VA_ARGS__))
#define cf_warning(context, __msg, ...) (cf_fault_event((context), CF_THREAD, CF_WARNING, __FILE__, __LINE__, (__msg), ##__VA_ARGS__))
#define cf_info(context, __msg, ...) (cf_fault_event((context), CF_THREAD, CF_INFO, __FILE__, __LINE__, (__msg), ##__VA_ARGS__))
#define cf_debug(context, __msg, ...) (cf_fault_event((context), CF_THREAD, CF_DEBUG, __FILE__, __LINE__, (__msg), ##__VA_ARGS__))
#define cf_detail(context, __msg, ...) (cf_fault_event((context), CF_THREAD, CF_DETAIL, __FILE__, __LINE__, (__msg), ##__VA_ARGS__))
extern char *cf_strerror(const int err);
extern int cf_fault_recovery_globalinit(cf_fault_recovery_key *rkey);
extern int cf_fault_recovery_localinit(cf_fault_recovery_key *rkey, cf_fault_recovery_stack *stack);
extern int cf_fault_recovery_push(cf_fault_recovery_key *rkey, void (*fn)(void *), void *arg);
extern int cf_fault_recovery_pop(cf_fault_recovery_key *rkey, int exec);
extern void cf_fault_recovery(cf_fault_recovery_key *rkey);
extern void cf_fault_init(const int argc, char **argv, const int excludec, char **exclude);
