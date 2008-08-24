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

/* All faults have an associated severity and scope */

/* cf_fault_severity[_strings]
 * The severity of a fault event
 *     ERROR               fatal startup errors
 *     CRITICAL            fatal runtime panics
 *     WARNING             runtime errors
 *     NOTICE              non-error special cases
 *     INFO                informational or advisory messages
 *     DEBUG               debugging messages
 */
typedef enum {
	CF_FAULT_SEVERITY_ERROR,
	CF_FAULT_SEVERITY_CRITICAL,
	CF_FAULT_SEVERITY_WARNING,
	CF_FAULT_SEVERITY_NOTICE,
	CF_FAULT_SEVERITY_INFO,
	CF_FAULT_SEVERITY_DEBUG
} cf_fault_severity;

/* cf_fault_scope[_strings]
 * The scope of a fault event
 *     GLOBAL              fatal errors terminate all execution
 *     PROCESS             fatal errors terminate the process
 *     THREAD              fatal errors terminate the enclosing thread
 */
typedef enum {
	CF_FAULT_SCOPE_GLOBAL,
	CF_FAULT_SCOPE_PROCESS,
	CF_FAULT_SCOPE_THREAD
} cf_fault_scope;


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
#define cf_fault_stop() abort()
extern void cf_fault_event(const cf_fault_scope, const cf_fault_severity, const char *fn, const int line, char *msg, ...);
#define cf_assert(a, severity, scope, msg, ...) ((void)((a) ? (void)0 : cf_fault_event(severity, scope, __func__, __LINE__, msg, ##__VA_ARGS__)))
extern char *cf_strerror(const int err);
extern int cf_fault_recovery_globalinit(cf_fault_recovery_key *rkey);
extern int cf_fault_recovery_localinit(cf_fault_recovery_key *rkey, cf_fault_recovery_stack *stack);
extern int cf_fault_recovery_push(cf_fault_recovery_key *rkey, void (*fn)(void *), void *arg);
extern int cf_fault_recovery_pop(cf_fault_recovery_key *rkey, int exec);
extern void cf_fault_recovery(cf_fault_recovery_key *rkey);
extern void cf_fault_init(const int argc, char **argv);
