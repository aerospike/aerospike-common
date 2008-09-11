/*
 *  Citrusleaf Foundation
 *  src/socket.c - socket functions
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



/* cf_socket_set_nonblocking
 * Set a socket to nonblocking mode */
int
cf_socket_set_nonblocking(int s)
{
	int flags;

	if (-1 == (flags = fcntl(s, F_GETFL, 0)))
		flags = 0;
	if (-1 == fcntl(s, F_SETFL, flags | O_NONBLOCK)) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "fcntl(): %s", cf_strerror(errno));
		return(0);
	}

	return(1);
}


/* cf_socket_recv
 * Read from a service socket */
int
cf_socket_recv(int sock, void *buf, size_t buflen, int flags)
{
	int i;

	if (0 >= (i = recv(sock, buf, buflen, flags))) {
		if (ECONNRESET == errno || 0 == i)
			cf_fault_event(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_INFO, NULL, 0, "socket disconnected");
		else
			cf_fault_event(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_INFO, NULL, 0, "recv() failed: %s", cf_strerror(errno));
	}

	return(i);
}


/* cf_socket_send
 * Send to a socket */
int
cf_socket_send(int sock, void *buf, size_t buflen, int flags)
{
	int i;

	if (0 >= (i = send(sock, buf, buflen, flags)))
		cf_fault_event(CF_FAULT_SCOPE_THREAD, CF_FAULT_SEVERITY_INFO, NULL, 0, "send() failed: %s", cf_strerror(errno));

	return(i);
}


/* cf_socket_init_svc
 * Initialize a socket for listening */
int
cf_socket_init_svc(cf_socket_cfg *s)
{
	struct timespec delay;

	cf_assert(s, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid argument");

	delay.tv_sec = 5;
	delay.tv_nsec = 0;

	/* Create the socket */
	if (0 > (s->sock = socket(AF_INET, SOCK_STREAM, 0))) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "socket: %s", cf_strerror(errno));
		return(errno);
	}
	s->saddr.sin_family = AF_INET;
	inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr);
	s->saddr.sin_port = htons(s->port);

	/* Bind to the socket; if we can't, nanosleep() and retry */
	while (0 > (bind(s->sock, (struct sockaddr *)&s->saddr, sizeof(struct sockaddr)))) {
		if (EADDRINUSE != errno) {
			cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "bind: %s", cf_strerror(errno));
			return(errno);
		}

		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_INFO, NULL, 0, "bind: socket in use, waiting");

		nanosleep(&delay, NULL);
	}

	/* Listen for connections */
	if (0 > listen(s->sock, 512)) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "listen: %s", cf_strerror(errno));
		return(errno);
	}

	/* Unblock the socket */
	if (-1 == cf_socket_set_nonblocking(s->sock))
		return(-1);

	return(0);
}


/* cf_socket_init_client
 * Connect a socket to a remote endpoint */
int
cf_socket_init_client(cf_socket_cfg *s)
{
	cf_assert(s, CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_CRITICAL, "invalid argument");

	if (0 > (s->sock = socket(AF_INET, SOCK_STREAM, 0))) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "socket: %s", cf_strerror(errno));
		return(errno);
	}
	
	memset(&s->saddr,0,sizeof(s->saddr));
	s->saddr.sin_family = AF_INET;
	if (0 >= inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr)) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "inet_pton: %s", cf_strerror(errno));
		close(s->sock);
		return(errno);
	}
	s->saddr.sin_port = htons(s->port);

	if (0 > (connect(s->sock, (struct sockaddr *)&s->saddr, sizeof(s->saddr)))) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "connect: %s", cf_strerror(errno));
		close(s->sock);
		return(errno);
	}

	return(0);
}


/* cf_svcmsocket_init
 * Initialize a multicast service/receive socket */
int
cf_mcastsocket_init(cf_mcastsocket_cfg *ms)
{
	cf_socket_cfg *s = &(ms->s);

	if (0 > (s->sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		fprintf(stderr, "mcast socket open errno %d",errno);
		return(errno);
	}

	/* want to allow multiple listners on the same port? Could be cool. Looks like:
	uint yes=1;
 	if (setsockopt(s->sock, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(yes))<0) {
		fprintf(stderr, "reuse of mcast socket failed errno %d\n",errno);
		return(errno);
	}
	*/

	memset(&s->saddr, 0, sizeof(s->saddr));
	s->saddr.sin_family = AF_INET;
	if (0 >= inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr)) {
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "inet_pton: %s", cf_strerror(errno));
		close(s->sock);
		return(errno);
	}
	s->saddr.sin_port = htons(s->port);

	while (0 > (bind(s->sock, (struct sockaddr *)&s->saddr, sizeof(struct sockaddr))))
		cf_fault_event(CF_FAULT_SCOPE_PROCESS, CF_FAULT_SEVERITY_WARNING, NULL, 0, "mcase bind: %s", cf_strerror(errno));

	inet_pton(AF_INET, s->addr, &ms->ireq.imr_multiaddr.s_addr);
	ms->ireq.imr_interface.s_addr = htonl(INADDR_ANY);
	setsockopt(s->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&ms->ireq, sizeof(struct ip_mreq));

	return(0);
}

void
cf_mcastsocket_close(cf_mcastsocket_cfg *ms)
{
	cf_socket_cfg *s = &(ms->s);

	close(s->sock);
}