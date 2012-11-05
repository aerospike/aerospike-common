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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
 
/* MUST BE KEPT IN SYNC WITH FAULT.H */
 
char *cf_fault_context_strings[] = {
	"cf:misc",     // 00
	"cf:alloc",    // 01
	"cf:hash",     // 02
	"cf:rchash",   // 03
	"cf:shash",    // 04
	"cf:queue",    // 05
	"cf:msg",      // 06
	"cf:redblack", // 07
	"cf:socket",   // 08
	"cf:timer",    // 09
	"cf:ll",       // 10
	"cf:arenah",   // 11
	"cf:arena",    // 12
	"config",      // 13
	"namespace",   // 14
	"as",          // 15
	"bin",         // 16
	"record",      // 17
	"proto",       // 18
	"particle",    // 19
	"demarshal",   // 20
	"write",       // 21
	"rw",          // 22
	"tsvc",        // 23
	"test",        // 24
	"nsup",        // 25
	"proxy",       // 26
	"hb",          // 27
	"fabric",      // 28
	"partition",   // 29
	"paxos",       // 30
	"migrate",     // 31
	"info",        // 32
	"info-port",   // 33
	"storage",     // 34
	"drv_mem",     // 35
	"drv_fs",      // 36
	"drv_files",   // 37
	"drv_ssd",     // 38
	"drv_kv",      // 39
	"scan",        // 40
	"index",       // 41
	"batch",       // 42
	"trial",       // 43
	"xdr",         // 44
	"cf:rbuffer",  // 45
	"fb_health",   // 46
	"sindex",      // 47
	"sproc",	   // 48
	NULL           // 49
};

static const char *cf_fault_severity_strings[] = { "CRITICAL", "WARNING", "INFO", "DEBUG", "DETAIL", NULL };
static const char *cf_fault_scope_strings[] = { "GLOBAL", "PROCESS", "THREAD" };

#define CF_FAULT_SINKS_MAX 8
cf_fault_sink cf_fault_sinks[CF_FAULT_SINKS_MAX];
cf_fault_severity cf_fault_filter[CF_FAULT_CONTEXT_UNDEF];
int cf_fault_sinks_inuse = 0;

/* cf_context_at_severity
 * Return whether the given context is set to this severity level or higher. */
bool
cf_context_at_severity(const cf_fault_context context, const cf_fault_severity severity)
{
	return (severity <= cf_fault_filter[context]);
}

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
	cf_assert((NULL != rkey || NULL != stack), CF_MISC, CF_PROCESS, CF_CRITICAL, "invalid argument");

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

    cf_fault_restart_argv = cf_malloc((argc + 2) * sizeof(char *));
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
            cf_fault_restart_argv[i] = cf_malloc((strlen(argv[j]) + 1) * sizeof(char));
            cf_assert(cf_fault_restart_argv[i], CF_MISC, CF_GLOBAL, CF_CRITICAL, "alloc: %s", cf_strerror(errno));
            memcpy(cf_fault_restart_argv[i], argv[j], strlen(argv[j]) + 1);
            i++;
        }
    }
	cf_fault_restart_argv[i] = cf_strdup("--process-fault");
	cf_fault_restart_argv[i + 1] = NULL;
    cf_fault_restart_argc = i + 1;

	/* Initialize the fault filter while we're here */
	for (int j = 0; j < CF_FAULT_CONTEXT_UNDEF; j++) {
		cf_fault_filter[j] = CF_CRITICAL;
	}

	return;
}


/* cf_fault_sink_add
 * Register an sink for faults */
cf_fault_sink *
cf_fault_sink_add(char *path)
{
	cf_fault_sink *s;

	if ((CF_FAULT_SINKS_MAX - 1) == cf_fault_sinks_inuse)
		return(NULL);

	s = &cf_fault_sinks[cf_fault_sinks_inuse++];
    s->path = cf_strdup(path);
	if (0 == strncmp(path, "stderr", 6))
		s->fd = 2;
	else {
		if (-1 == (s->fd = open(path, O_WRONLY|O_CREAT|O_APPEND|O_NONBLOCK, S_IRUSR|S_IWUSR))) {
			cf_fault_sinks_inuse--;
			return(NULL);
		}
	}

	for (int i = 0; i < CF_FAULT_CONTEXT_UNDEF; i++)
		s->limit[i] = CF_INFO;

	return(s);
}


/* cf_fault_sink_get
 * Get a fault sink pointer from a path */
cf_fault_sink *
cf_fault_sink_get(char *path)
{
    cf_fault_sink *s = NULL;

    for (int i = 0; i < CF_FAULT_SINKS_MAX; i++) {
        cf_fault_sink *t = &cf_fault_sinks[i];
        if (0 == strncmp(path, t->path, strlen(t->path))) {
            s = t;
            break;
        }
    }

    return(s);
}


int
cf_fault_sink_addcontext_all(char *context, char *severity)
{
    for (int i = 0; i < cf_fault_sinks_inuse; i++) {
        cf_fault_sink *s = &cf_fault_sinks[i];
        int rv = cf_fault_sink_addcontext(s, context, severity);
        if (rv != 0)	return(rv);
    }
    return(0);
}


int
cf_fault_sink_addcontext(cf_fault_sink *s, char *context, char *severity)
{
	if (s == 0) 		return(cf_fault_sink_addcontext_all(context, severity));

	cf_fault_context ctx = CF_FAULT_CONTEXT_UNDEF;
	cf_fault_severity sev = CF_FAULT_SEVERITY_UNDEF;

	for (int i = 0; i < CF_FAULT_SEVERITY_UNDEF; i++) {
		if (0 == strncasecmp(cf_fault_severity_strings[i], severity, strlen(severity)))
			sev = (cf_fault_severity)i;
	}
	if (CF_FAULT_SEVERITY_UNDEF == sev)
		return(-1);

	if (0 == strncasecmp(context, "any", 3)) {
		for (int i = 0; i < CF_FAULT_CONTEXT_UNDEF; i++) {
			s->limit[i] = sev;
			if (s->limit[i] > cf_fault_filter[i])
				cf_fault_filter[i] = s->limit[i];
		}
	} else {
		for (int i = 0; i < CF_FAULT_CONTEXT_UNDEF; i++) {
		//strncasecmp only compared the length of context passed in the 3 rd argument and as cf_fault_context_strings has info and info port,
		//So when you try to set info to debug it will set info-port to debug . Just forcing it to check the length from cf_fault_context_strings
			if (0 == strncasecmp(cf_fault_context_strings[i], context, strlen(cf_fault_context_strings[i])))
				ctx = (cf_fault_context)i;
		}
		if (CF_FAULT_CONTEXT_UNDEF == ctx)
			return(-1);

		s->limit[ctx] = sev;
		if (s->limit[ctx] > cf_fault_filter[ctx])
			cf_fault_filter[ctx] = s->limit[ctx];
	}

	return(0);
}

int
cf_fault_sink_setcontext(cf_fault_sink *s, char *context, char *severity)
{
	if (s == 0) 		return(cf_fault_sink_addcontext_all(context, severity));

	cf_fault_context ctx = CF_FAULT_CONTEXT_UNDEF;
	cf_fault_severity sev = CF_FAULT_SEVERITY_UNDEF;

	for (int i = 0; i < CF_FAULT_SEVERITY_UNDEF; i++) {
		if (0 == strncasecmp(cf_fault_severity_strings[i], severity, strlen(severity)))
			sev = (cf_fault_severity)i;
	}
	if (CF_FAULT_SEVERITY_UNDEF == sev)
		return(-1);

	for (int i = 0; i < CF_FAULT_CONTEXT_UNDEF; i++) {
		if (0 == strncasecmp(cf_fault_context_strings[i], context, strlen(cf_fault_context_strings[i])))
			ctx = (cf_fault_context)i;
	}
	if (CF_FAULT_CONTEXT_UNDEF == ctx)
		return(-1);

	s->limit[ctx] = sev;
	cf_fault_filter[ctx] = s->limit[ctx];
	return(0);
}


/* cf_fault_event
 * Respond to a fault */
void
cf_fault_event(const cf_fault_context context, const cf_fault_scope scope, const cf_fault_severity severity, const char *fn, const int line, char *msg, ...)
{

	/* Prefilter: don't construct messages we won't end up writing */
	if (severity > cf_fault_filter[context])
		return;

	va_list argp;
	char mbuf[1024];
	time_t now;
	struct tm nowtm;
	void *bt[CF_FAULT_BACKTRACE_DEPTH];
	char **btstr;
	int btn;
	
	/* Make sure there's always enough space for the \n\0. */
	size_t limit = sizeof(mbuf) - 2;

	/* Set the timestamp */
	now = time(NULL);
	gmtime_r(&now, &nowtm);
	size_t pos = strftime(mbuf, limit, "%b %d %Y %T %Z: ", &nowtm);

	/* Set the context/scope/severity tag */
	if (CF_CRITICAL == severity)
		pos += snprintf(mbuf + pos, limit - pos, "%s %s (%s): ", cf_fault_severity_strings[severity], cf_fault_scope_strings[scope], cf_fault_context_strings[context]);
	else
		pos += snprintf(mbuf + pos, limit - pos, "%s (%s): ", cf_fault_severity_strings[severity], cf_fault_context_strings[context]);

	/*
	 * snprintf() and vsnprintf() will not write more than the size specified,
	 * but they return the size that would have been written without truncation.
	 * These checks make sure there's enough space for the final \n\0.
	 */
	if (pos > limit) {
		pos = limit;
	}

	/* Set the location */
	if (fn)
		pos += snprintf(mbuf + pos, limit - pos, "(%s:%d) ", fn, line);

	if (pos > limit) {
		pos = limit;
	}

	/* Append the message */
	va_start(argp, msg);
	pos += vsnprintf(mbuf + pos, limit - pos, msg, argp);
	va_end(argp);

	if (pos > limit) {
		pos = limit;
	}

	pos += snprintf(mbuf + pos, 2, "\n");

	/* Route the message to the correct destinations */
	if (0 == cf_fault_sinks_inuse) {
	    /* If no fault sinks are defined, use stderr for critical messages */
		if (CF_CRITICAL == severity)
			fprintf(stderr, "%s", mbuf);
	} else {
		for (int i = 0; i < cf_fault_sinks_inuse; i++) {
			if ((severity <= cf_fault_sinks[i].limit[context]) || (CF_CRITICAL == severity)) {
				if (0 >= write(cf_fault_sinks[i].fd, mbuf, pos)) {
					// this is OK for a bit in case of a HUP. It's even better to queue the buffers and apply them
					// after the hup. TODO.
					fprintf(stderr, "internal failure in fault message write: %s\n", cf_strerror(errno));
				}
			}
		}
	}

	/* Critical errors */
	if (CF_CRITICAL == severity) {
		fflush(NULL);

		int wb = 0;
		
		btn = backtrace(bt, CF_FAULT_BACKTRACE_DEPTH);
		btstr = backtrace_symbols(bt, btn);
		if (!btstr) {
			for (int i = 0; i < cf_fault_sinks_inuse; i++) {
				char *no_bkstr = " --- NO BACKTRACE AVAILABLE --- \n";
				wb += write(cf_fault_sinks[i].fd, no_bkstr, strlen(no_bkstr));
			}
		}
		else {
			for (int i = 0; i < cf_fault_sinks_inuse; i++) {
				for (int j=0; j < btn; j++) {
					char line[60];
					sprintf(line, "critical error: backtrace: frame %d ",j);
					wb += write(cf_fault_sinks[i].fd, line, strlen(line));
					wb += write(cf_fault_sinks[i].fd, btstr[j], strlen(btstr[j]));
					wb += write(cf_fault_sinks[i].fd, "\n", 1);
				}
			}
		}

		switch(scope) {
			case CF_GLOBAL:
				abort();
				break;
			case CF_PROCESS:
				// having process failures attempt restart is sometimes very confusing
				abort();
#if 0				
				if (-1 == execvp(cf_fault_restart_argv[0], cf_fault_restart_argv))
					cf_assert(NULL, CF_MISC, CF_GLOBAL, CF_CRITICAL, "execvp failed: %s", cf_strerror(errno));
#endif				
				break;
			case CF_THR:
			    /* Since we may have asynchronous cancellation disabled,
				 * we always force a check for cancellation */
				pthread_cancel(pthread_self());
				pthread_testcancel();
				break;
		}
	}

	return;
}

int
cf_fault_sink_strlist(cf_dyn_buf *db)
{
	for (int i = 0; i < cf_fault_sinks_inuse; i++) {
		cf_dyn_buf_append_int(db, i);
		cf_dyn_buf_append_char(db, ':');
		cf_dyn_buf_append_string(db,cf_fault_sinks[i].path);
		cf_dyn_buf_append_char(db, ';');
	}
	cf_dyn_buf_chomp(db);
	return(0);
}


extern void
cf_fault_sink_logroll(void)
{
	fprintf(stderr, "cf_fault: rolling log files\n");
	for (int i = 0; i < cf_fault_sinks_inuse; i++) {
		cf_fault_sink *s = &cf_fault_sinks[i];
		if ((0 != strncmp(s->path,"stderr", 6)) && (s->fd > 2)) {

			int fd = s->fd;
			s->fd = -1;
			usleep(1);

			// hopefully, the file has been relinked elsewhere - or you're OK losing it
			unlink(s->path);
			close(fd);
			
			fd = open(s->path, O_WRONLY|O_CREAT|O_NONBLOCK|O_APPEND, S_IRUSR|S_IWUSR);
			s->fd = fd;
		}

	}
}


cf_fault_sink *cf_fault_sink_get_id(int id)
{
	if (id > cf_fault_sinks_inuse)	return(0);
	return ( &cf_fault_sinks[id] );
	
}

int 
cf_fault_sink_context_all_strlist(int sink_id, cf_dyn_buf *db)
{
	// get the sink
	if (sink_id > cf_fault_sinks_inuse)	return(-1);
	cf_fault_sink *s = &cf_fault_sinks[sink_id];
	
	for (uint i=0; i<CF_FAULT_CONTEXT_UNDEF; i++) {
		cf_dyn_buf_append_string(db, cf_fault_context_strings[i]);
		cf_dyn_buf_append_char(db, ':');
		cf_dyn_buf_append_string(db, cf_fault_severity_strings[s->limit[i]]);
		cf_dyn_buf_append_char(db, ';');
	}
	cf_dyn_buf_chomp(db);
	return(0);
}

int 
cf_fault_sink_context_strlist(int sink_id, char *context, cf_dyn_buf *db)
{
	// get the sink
	if (sink_id > cf_fault_sinks_inuse)	return(-1);
	cf_fault_sink *s = &cf_fault_sinks[sink_id];
	
	// get the severity
	uint i;
	for (i=0;i<CF_FAULT_CONTEXT_UNDEF;i++) {
		if (0 == strcmp(cf_fault_context_strings[i],context))
			break;
	}
	if (i == CF_FAULT_CONTEXT_UNDEF) {
		cf_dyn_buf_append_string(db, context);
		cf_dyn_buf_append_string(db, ":unknown");
		return(0);
	}
		
	// get the string	
	cf_dyn_buf_append_string(db, context);
	cf_dyn_buf_append_char(db, ':');
	cf_dyn_buf_append_string(db, cf_fault_severity_strings[s->limit[i]]);
	return(0);	
}


