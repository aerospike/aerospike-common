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
char **cf_fault_restart_argv;


/* cf_fault_severity_strings, cf_fault_scope_strings
 * Strings describing the severity and scope states */
static const char *cf_fault_severity_strings[] = { "ERROR", "CRITICAL", "WARNING", "NOTICE", "INFO", "DEBUG" };
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
	cf_assert(rkey, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid arguments");

	return(pthread_key_create(rkey, NULL));
}


/* cf_fault_recovery_localinit
 * Initialize the local half of the thread recovery system */
int
cf_fault_recovery_localinit(cf_fault_recovery_key *rkey, cf_fault_recovery_stack *stack)
{
	if (NULL == rkey || NULL == stack)
		cf_assert(NULL, CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_CRITICAL, "invalid arguments");

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

	if (NULL == rkey || NULL == fn)
		cf_assert(NULL, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid arguments");
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

	cf_assert(rkey, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid argument");
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

	cf_assert(rkey, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid argument");
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
 * executed by main() and failure MUST result in a global fault */
void
cf_fault_init(const int argc, char **argv)
{
	int i;

	if (0 == argc || NULL == argv)
		cf_assert(NULL, CF_FAULT_SCOPE_GLOBAL, CF_FAULT_SEVERITY_CRITICAL, "invalid arguments");

	/* Copy the argument vector */
	cf_fault_restart_argv = calloc(argc, sizeof(char *));
	cf_assert(cf_fault_restart_argv, CF_FAULT_SCOPE_GLOBAL, CF_FAULT_SEVERITY_CRITICAL, "calloc: %s", cf_strerror(errno));
	for (i = 0; i < argc; i++) {
		cf_fault_restart_argv[i] = calloc(strlen(argv[i]), sizeof(char));
		cf_assert(cf_fault_restart_argv[i], CF_FAULT_SCOPE_GLOBAL, CF_FAULT_SEVERITY_CRITICAL, "calloc: %s", cf_strerror(errno));
		memcpy(cf_fault_restart_argv[i], argv[i], strlen(argv[i]));
	}

	return;
}


/* cf_fault_event
 * Respond to a fault */
void
cf_fault_event(const cf_fault_scope scope, const cf_fault_severity severity, const char *fn, const int line, char *msg, ...)
{
	va_list argp;
	char mbuf[2048];
	time_t now;
	struct tm nowtm;
/*
	void *bt[CF_FAULT_BACKTRACE_DEPTH];
	char **btstr = NULL;
	size_t btsz;
	int i;
*/

	/* Generate a timestamp */
	now = time(NULL);
	gmtime_r(&now, &nowtm);
	strftime(mbuf, sizeof(mbuf), "%b %d %Y %T: ", &nowtm);

	/* Get the backtrace information */
/*
	btsz = backtrace(bt, CF_FAULT_BACKTRACE_DEPTH);
	btstr = backtrace_symbols(bt, btsz);
*/

	/* Construct the scope/severity tag */
	if (CF_FAULT_SEVERITY_CRITICAL >= severity)
		snprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), "[%s:%s] ", cf_fault_severity_strings[scope], cf_fault_scope_strings[severity]);
	else
		snprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), "[%s] ", cf_fault_severity_strings[severity]);

	/* Construct the location tag, if a location was provided */
	if (fn)
		snprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), "(%s:%d) ", fn, line);

	/* Append the message */
	va_start(argp, msg);
	vsnprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), msg, argp);
	va_end(argp);

	/* Send the mbuf to stderr and take the appropriate action */
	fprintf(stderr, "%s\n", mbuf);

	/* Take the appropriate action */
	if (CF_FAULT_SEVERITY_CRITICAL >= severity) {
		/* Dump a backtrace if the error is fatal */
/*		if (NULL != btstr)
			for (i = 0; i < btsz; i++)
				fprintf(stderr, "  %s\n", btstr[i]);
*/

		switch(scope) {
			case CF_FAULT_SCOPE_GLOBAL:
				abort();
				break;
			case CF_FAULT_SCOPE_PROCESS:
				if (-1 == execvp(cf_fault_restart_argv[0], cf_fault_restart_argv))
					cf_fault_event(CF_FAULT_SCOPE_GLOBAL, CF_FAULT_SEVERITY_ERROR, NULL, 0, "execvp failed: %s", cf_strerror(errno));
				break;
			case CF_FAULT_SCOPE_THREAD:
				/* Since we may have asynchronous cancellation disabled,
				 * we always force a check for cancellation */
				pthread_cancel(pthread_self());
				pthread_testcancel();
				break;
		}
	}

	return;
}
