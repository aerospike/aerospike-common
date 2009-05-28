/*
 *  Citrusleaf Foundation
 *  src/process.c - process utilities
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
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"



void
cf_process_privsep(uid_t uid, gid_t gid)
{
    if ((0 != getuid() || ((uid == getuid()) && (gid == getgid()))))
        return;

    /* Drop all auxiliary groups */
    if (0 > setgroups(0, (const gid_t *)0))
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "setgroups: %s", cf_strerror(errno));

    /* Change privileges */
    if (0 > setgid(gid))
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "setgid: %s", cf_strerror(errno));
    if (0 > setuid(uid))
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "setuid: %s", cf_strerror(errno));

    return;
}



void
cf_process_daemonize(uid_t uid, gid_t gid)
{
    int FD;
    char cfile[128];
    pid_t p;

    /* Fork ourselves, then let the parent expire */
    if (-1 == (p = fork()))
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "couldn't fork: %s", cf_strerror(errno));
    if (0 != p)
        exit(0);

    /* Get a new session */
    if (-1 == setsid())
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "couldn't set session: %s", cf_strerror(errno));

    /* Drop all the file descriptors */
    for (int i = getdtablesize(); i > 2; i--)
        close(i);

    /* Open a temporary file for console message redirection */
    snprintf(cfile, 128, "/tmp/aerospike-console.%d", getpid());
    if (-1 == (FD = open(cfile, O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR)))
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "couldn't open console redirection file: %s", cf_strerror(errno));
    if (-1 == chmod(cfile, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)))
        cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "couldn't set mode on console redirection file: %s", cf_strerror(errno));

    /* Redirect stdout, stderr, and stdin to the console file */
    for (int i = 0; i < 3; i++) {
        if (-1 == dup2(FD, i))
            cf_crash(CF_MISC, CF_GLOBAL, CF_CRITICAL, "couldn't duplicate FD: %s", cf_strerror(errno));
    }

     return;
}
