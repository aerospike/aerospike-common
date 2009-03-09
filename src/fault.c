/*
 *  Citrusleaf Foundation
 *  src/fault.c - fault management framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"


/* SYNOPSIS
 * */



/* cf_fault_restart_process, cf_fault_restart_argv
 * Global variables for process restart state */
int cf_fault_restart_argc;
char **cf_fault_restart_argv;


/* cf_fault_context_strings, cf_fault_severity_strings, cf_fault_scope_strings
 * Strings describing fault states */
static const char *cf_fault_context_strings[] = {
	"cf:misc", "cf:rcalloc", "cf:hash", "cf:rchash", "cf:shash", "cf:queue", "cf:msg", "cf:redblack", "cf:socket",
	"config", "namespace", "as", "bin", "record", "proto", "particle", "demarshal", "write", "tsvc", "test", "nsup", "proxy", "hb", "fabric", "partition", "paxos", "migrate"
};

static const char *cf_fault_severity_strings[] = { "CRITICAL", "WARNING", "INFO", "DEBUG", "DETAIL" };
static const char *cf_fault_scope_strings[] = { "GLOBAL", "PROCESS", "THREAD" };


/* cf_strerror
 * Some platforms return the errno in the string if the errno's value is
 * unknown: this is traditionally done with a static buffer.  Unfortunately,
 * this causes strerror to not be thread-safe.  cf_strerror() acts properly
 * and simply returns "Unknown error", avoiding thread safety issues */
char *
cf_strerror(const int err)
{
	if (err < sys_nerr && err >= 0)
		return ((char *)sys_errlist[err]);

	errno = EINVAL;
	return("Unknown error");
}


/* cf_fault_recovery_globalinit
 * Initialize the global half of the thread recovery system */
int
cf_fault_recovery_globalinit(cf_fault_recovery_key *rkey)
{
	cf_assert(rkey, CF_MISC, CF_PROCESS, CF_CRITICAL, "null rkey");

	return(pthread_key_create(rkey, NULL));
}


/* cf_fault_recovery_localinit
 * Initialize the local half of the thread recovery system */
int
cf_fault_recovery_localinit(cf_fault_recovery_key *rkey, cf_fault_recovery_stack *stack)
{
	cf_assert((NULL != rkey || NULL != stack), CF_MISC, CF_THREAD, CF_CRITICAL, "invalid argument");

	/* Set the stack attributes */
	stack->sz = -1;

	/* Set the key */
	if (0 != pthread_setspecific(*rkey, (void *)stack))
		return(-1);

	return(0);
}


/* cf_fault_recovery_push
 * Push a function and argument onto a recovery stack */
int
cf_fault_recovery_push(cf_fault_recovery_key *rkey, void (*fn)(void *), void *arg)
{
	cf_fault_recovery_stack *stack;

	cf_assert((NULL != rkey || NULL != fn), CF_MISC, CF_PROCESS, CF_CRITICAL, "invalid argument");
	if (NULL == (stack = (cf_fault_recovery_stack *)pthread_getspecific(*rkey)))
		return(-1);

	/* Check for stack overflow */
	if (CF_FAULT_RECOVERY_STACK_MAXDEPTH <= stack->sz)
		return(-1);

	/* Put the new element on the stack */
	stack->sz++;
	stack->fn[stack->sz] = fn;
	stack->arg[stack->sz] = arg;

	return(0);
}


/* cf_fault_recovery_pop
 * Pop a function and argument off a recovery stack */
int
cf_fault_recovery_pop(cf_fault_recovery_key *rkey, int exec)
{
	cf_fault_recovery_stack *stack;
	cf_assert(rkey, CF_MISC, CF_PROCESS, CF_CRITICAL, "invalid argument");

	if (NULL == (stack = (cf_fault_recovery_stack *)pthread_getspecific(*rkey)))
		return(-1);

	/* Execute the last function on the stack, if appropriate, and decrement
	 * the stack */
	if (exec)
		stack->fn[stack->sz](stack->arg[stack->sz]);
	stack->sz--;

	return(0);
}


/* cf_fault_recovery
 * Execute a recovery stack */
void
cf_fault_recovery(cf_fault_recovery_key *rkey)
{
	cf_fault_recovery_stack *stack;
	cf_assert(rkey, CF_MISC, CF_PROCESS, CF_CRITICAL, "invalid argument");

	if (NULL == (stack = (cf_fault_recovery_stack *)pthread_getspecific(*rkey)))
		return;

	/* Run the stack */
	for ( ; -1 != stack->sz; stack->sz--)
		stack->fn[stack->sz](stack->arg[stack->sz]);

	return;
}


/* cf_fault_init
 * Copy the initial arguments so that we can restart in the event that we
 * sustain a fault in process scope.  This code MUST be the first thing
 * executed by main() and failure MUST result in a global fault.
 * Arguments that we were passed that are in the excluded arguments list
 * will not be used on a restart */
void
cf_fault_init(const int argc, char **argv, const int excludec, char **exclude)
{
	int i = 0;
	cf_assert((0 != argc || NULL != argv), CF_MISC, CF_GLOBAL, CF_CRITICAL, "invalid argument");

    cf_fault_restart_argv = malloc((argc + 2) * sizeof(char *));
	cf_assert(cf_fault_restart_argv, CF_MISC, CF_GLOBAL, CF_CRITICAL, "alloc: %s", cf_strerror(errno));

    for (int j = 0; j < argc; j++) {
        bool copy = true;
        for (int k = 0; k < excludec; k++) {
            if (0 == memcmp(argv[j], exclude[k], strlen(argv[j]))) {
                copy = false;
                break;
            }
        }

        if (copy) {
            cf_fault_restart_argv[i] = malloc((strlen(argv[j]) + 1) * sizeof(char));
            cf_assert(cf_fault_restart_argv[i], CF_MISC, CF_GLOBAL, CF_CRITICAL, "alloc: %s", cf_strerror(errno));
            memcpy(cf_fault_restart_argv[i], argv[j], strlen(argv[j]) + 1);
            i++;
        }
    }
	cf_fault_restart_argv[i] = strdup("--process-fault");
	cf_fault_restart_argv[i + 1] = NULL;
    cf_fault_restart_argc = i + 1;

	return;
}


/* cf_fault_event
 * Respond to a fault */
void
cf_fault_event(const cf_fault_context context, const cf_fault_scope scope, const cf_fault_severity severity, const char *fn, const int line, char *msg, ...)
{
	va_list argp;
	char mbuf[256];
	time_t now;
	struct tm nowtm;
	void *bt[CF_FAULT_BACKTRACE_DEPTH];
	char **btstr;
	int btn;

	if (context == AS_MIGRATE) {
		if (CF_DEBUG == severity || CF_DETAIL == severity) return;
	}
	else {
		if (CF_DEBUG == severity || CF_DETAIL == severity) return;
	}
	
	/* Set the timestamp */
	now = time(NULL);
	gmtime_r(&now, &nowtm);
	strftime(mbuf, sizeof(mbuf), "%b %d %Y %T: ", &nowtm);

	/* Set the context/scope/severity tag */
	if (CF_CRITICAL == severity)
		snprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), "%s %s (%s): ", cf_fault_severity_strings[severity], cf_fault_scope_strings[scope], cf_fault_context_strings[context]);
	else
		snprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), "%s (%s): ", cf_fault_severity_strings[severity], cf_fault_context_strings[context]);

	/* Set the location */
	if (fn)
		snprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), "(%s:%d) ", fn, line);

	/* Append the message */
	va_start(argp, msg);
	vsnprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), msg, argp);
	va_end(argp);

	/* Route the message to the correct destinations */
	/* FIXME yeah, actually do that! */
	fprintf(stderr, "%s\n", mbuf);

	/* Critical errors */
	if (CF_CRITICAL == severity) {
		btn = backtrace(bt, CF_FAULT_BACKTRACE_DEPTH);
		btstr = backtrace_symbols(bt, btn);
		cf_assert(btstr, CF_MISC, CF_PROCESS, CF_CRITICAL, "backtrace_symbols() returned NULL");
		for (int i = 0; i < btn; i++)
			fprintf(stderr, "  %s\n", btstr[i]);

		switch(scope) {
			case CF_GLOBAL:
				abort();
				break;
			case CF_PROCESS:
				if (-1 == execvp(cf_fault_restart_argv[0], cf_fault_restart_argv))
					cf_assert(NULL, CF_MISC, CF_GLOBAL, CF_CRITICAL, "execvp failed: %s", cf_strerror(errno));
				break;
			case CF_THREAD:
			    /* Since we may have asynchronous cancellation disabled,
				 * we always force a check for cancellation */
				pthread_cancel(pthread_self());
				pthread_testcancel();
				break;
		}
	}

	return;
}
