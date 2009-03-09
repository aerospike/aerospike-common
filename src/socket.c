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

void
cf_sockaddr_convertto(const struct sockaddr_in *src, cf_sockaddr *dst)
{
	if (src->sin_family != AF_INET)	return;
	byte *b = (byte *) dst;
	memcpy(b, &(src->sin_addr.s_addr),4);
	memcpy(b+4,&(src->sin_port),2);
	memset(b+6,0,2);
}

void
cf_sockaddr_convertfrom(const cf_sockaddr src, struct sockaddr_in *dst)
{
	dst->sin_family = AF_INET;
	byte *b = (byte *) &src;
	
	memcpy(&dst->sin_addr.s_addr,b,4);
	memcpy(&dst->sin_port, b+4, 2);
}

void
cf_sockaddr_setport(cf_sockaddr *so, unsigned short port)
{
	byte *b = (byte *) so;
	port = htons(port);
	memcpy(b+4,&(port),2);	
}



/* cf_socket_set_nonblocking
 * Set a socket to nonblocking mode */
int
cf_socket_set_nonblocking(int s)
{
	int flags;

	if (-1 == (flags = fcntl(s, F_GETFL, 0)))
		flags = 0;
	if (-1 == fcntl(s, F_SETFL, flags | O_NONBLOCK)) {
		cf_crash(CF_SOCKET, CF_PROCESS,  "fcntl(): %s", cf_strerror(errno));
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
		if (EAGAIN == errno)
			return(0);
		else if (ECONNRESET == errno || 0 == i)
			cf_detail(CF_SOCKET, "socket disconnected");
		else {
			cf_crash(CF_SOCKET, CF_PROCESS, "recv() failed: %d %s", errno, cf_strerror(errno));
		}
			
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
		cf_warning(CF_SOCKET, "send() failed: %s", cf_strerror(errno));

	return(i);
}



/* cf_socket_recvfrom
 * Read from a service socket */
int
cf_socket_recvfrom(int sock, void *buf, size_t buflen, int flags, cf_sockaddr *from)
{
	int i;
	struct sockaddr_in f;
	socklen_t fl = sizeof(f);

	if (0 >= (i = recvfrom(sock, buf, buflen, flags, (struct sockaddr *) &f, &fl))) {
		cf_warning(CF_SOCKET, "recvfrom() failed: %s", cf_strerror(errno));
	}

	cf_sockaddr_convertto( &f, from); 

	return(i);
}


/* cf_socket_send
 * Send to a socket */
int
cf_socket_sendto(int sock, void *buf, size_t buflen, int flags, cf_sockaddr to)
{
	int i;
	struct sockaddr_in s;
	
	cf_sockaddr_convertfrom(to, &s);
	
	if (0 >= (i = sendto(sock, buf, buflen, flags, (struct sockaddr *) &s, sizeof(const struct sockaddr))))
		cf_info(CF_SOCKET, "send() failed: %s", cf_strerror(errno));

	return(i);
}

/* cf_socket_init_svc
 * Initialize a socket for listening 
 * Leaves the socket in a blocking state - if you want nonblocking, set nonblocking
 */
int
cf_socket_init_svc(cf_socket_cfg *s)
{
	struct timespec delay;
	cf_assert(s, CF_SOCKET, CF_PROCESS, CF_CRITICAL, "invalid argument");
	if (!s->addr) {
		cf_info(CF_SOCKET, "Could not initialize service, check config file");
		return(-1);
	}
	if (s->port == 0) {
		cf_info(CF_SOCKET, "could not initialize service, missing port, check config file");
		return(-1);
	}

	delay.tv_sec = 5;
	delay.tv_nsec = 0;

	/* Create the socket */
	if (0 > (s->sock = socket(AF_INET, SOCK_STREAM, 0))) {
		cf_crash(CF_SOCKET, CF_PROCESS, "socket: %s", cf_strerror(errno));
		return(errno);
	}
	s->saddr.sin_family = AF_INET;
	inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr);
	s->saddr.sin_port = htons(s->port);
	
	if (s->reuse_addr) {
		int v = 1;
		setsockopt(s->sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v) );
	}

	/* Set close-on-exec */
	fcntl(s->sock, F_SETFD, 1);

	/* Bind to the socket; if we can't, nanosleep() and retry */
	while (0 > (bind(s->sock, (struct sockaddr *)&s->saddr, sizeof(struct sockaddr)))) {
		if (EADDRINUSE != errno) {
			cf_warning(CF_SOCKET, "bind: %s", cf_strerror(errno));
			return(errno);
		}

		cf_warning(CF_SOCKET, "bind: socket in use, waiting");

		nanosleep(&delay, NULL);
	}

	/* Listen for connections */
	if (0 > listen(s->sock, 512)) {
		cf_warning(CF_SOCKET, "listen: %s", cf_strerror(errno));
		return(errno);
	}

	return(0);
}


/* cf_socket_init_client
 * Connect a socket to a remote endpoint */
int
cf_socket_init_client(cf_socket_cfg *s)
{
	cf_assert(s, CF_SOCKET, CF_PROCESS, CF_CRITICAL, "invalid argument");

	if (0 > (s->sock = socket(AF_INET, SOCK_STREAM, 0))) {
		cf_warning(CF_SOCKET, "socket: %s", cf_strerror(errno));
		return(errno);
	}

	/* Set close-on-exec */
	fcntl(s->sock, F_SETFD, 1);

	memset(&s->saddr,0,sizeof(s->saddr));
	s->saddr.sin_family = AF_INET;
	if (0 >= inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr)) {
		cf_warning(CF_SOCKET, "inet_pton: %s", cf_strerror(errno));
		close(s->sock);
		return(errno);
	}
	s->saddr.sin_port = htons(s->port);

	if (0 > (connect(s->sock, (struct sockaddr *)&s->saddr, sizeof(s->saddr)))) {
		cf_warning(CF_SOCKET, "connect: %s", cf_strerror(errno));
		close(s->sock);
		return(errno);
	}

	return(0);
}

/* cf_socket_init_client
 * Connect a socket to a remote endpoint
 * In the nonblocking fashion
 * returns the file descriptor
 */
int
cf_socket_connect_nb(cf_sockaddr so, int *fd_r)
{

	struct sockaddr_in sa;
	cf_sockaddr_convertfrom(so, &sa);
	
	int fd;	
	if (0 > (fd = socket(AF_INET, SOCK_STREAM, 0))) {
		cf_warning(CF_SOCKET, "socket connect error: %d %s", errno, cf_strerror(errno));
		return(errno);
	}

	/* Set close-on-exec */
	fcntl(fd, F_SETFD, 1);

	cf_socket_set_nonblocking(fd);
	
	if (0 > (connect(fd, (struct sockaddr *)&sa, sizeof(sa)))) {
		if (errno != EINPROGRESS) {
			cf_warning(CF_SOCKET, "socket connect error: %d %s", errno, cf_strerror(errno));
			close(fd);
			return(errno);
		}
	}

//	byte *b = (byte *) &sa.sin_addr;
//	cf_debug(CF_SOCKET,"creating connection: fd %d %02x.%02x.%02x.%02x : %d",fd, b[0],b[1],b[2],b[3] ,htons(sa.sin_port) );

	*fd_r = fd; 
	return(0);
}



/* cf_svcmsocket_init
 * Initialize a multicast service/receive socket
 * Bind is done to INADDR_ANY - all interfaces
 *  */
int
cf_mcastsocket_init(cf_mcastsocket_cfg *ms)
{
	cf_socket_cfg *s = &(ms->s);

	if (0 > (s->sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		cf_warning(CF_SOCKET, "multicast socket open error: %d %s", errno, cf_strerror(errno));
		return(errno);
	}

	cf_debug(CF_SOCKET, "mcast_socket init: socket %d",s->sock);

	// allows multiple readers on the same address
	uint yes=1;
 	if (setsockopt(s->sock, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(yes))<0) {
		cf_warning(CF_SOCKET, "multicast socket reuse failed: %d %s", errno, cf_strerror(errno));
		return(errno);
	}

	/* Set close-on-exec */
	fcntl(s->sock, F_SETFD, 1);

	// Bind to the incoming port on inaddr any
	memset(&s->saddr, 0, sizeof(s->saddr));
	s->saddr.sin_family = AF_INET;
	s->saddr.sin_addr.s_addr = INADDR_ANY;
	s->saddr.sin_port = htons(s->port);
	while (0 > (bind(s->sock, (struct sockaddr *)&s->saddr, sizeof(struct sockaddr))))
		cf_info(CF_SOCKET, "multicast socket bind failed: %d %s", errno, cf_strerror(errno));

	// Register for the multicast group
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
