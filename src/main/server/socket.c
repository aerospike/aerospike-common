/*
 *  Citrusleaf Foundation
 *  src/socket.c - socket functions
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include "server/cf.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <time.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>


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
	byte *b = (byte *) &src;

	dst->sin_family = AF_INET;
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
    flags |= MSG_NOSIGNAL;
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
    flags |= MSG_NOSIGNAL;
	if (0 >= (i = send(sock, buf, buflen, flags))) {
		cf_warning(CF_SOCKET, "send() failed: %d %s", errno, cf_strerror(errno));
	}

	return(i);
}



/* cf_socket_recvfrom
 * Read from a service socket */
int
cf_socket_recvfrom(int sock, void *buf, size_t buflen, int flags, cf_sockaddr *from)
{
	int i;
	struct sockaddr_in f, *fp = NULL;
	socklen_t fl = sizeof(f);

    if (from) {
        fp = &f;
        f.sin_family = AF_INET;
    }

    flags |= MSG_NOSIGNAL;
    
	if (0 >= (i = recvfrom(sock, buf, buflen, flags, (struct sockaddr *)fp, &fl))) {
		cf_warning(CF_SOCKET, "recvfrom() failed: %d %s", errno, cf_strerror(errno));
		if (from) memset(from, 0, sizeof(cf_sockaddr));
	}
    else{
    	if (from) 	    cf_sockaddr_convertto(fp, from);
    }

	return(i);
}


/* cf_socket_send
 * Send to a socket */
int
cf_socket_sendto(int sock, void *buf, size_t buflen, int flags, cf_sockaddr to)
{
	int i;
	struct sockaddr_in s, *sp = NULL;

    if (to) {
        sp = &s;
	    cf_sockaddr_convertfrom(to, sp);
	}
	
    flags |= MSG_NOSIGNAL;
    
	if (0 >= (i = sendto(sock, buf, buflen, flags, (struct sockaddr *)sp, sizeof(const struct sockaddr))))
		cf_info(CF_SOCKET, "sendto() failed: %d %s", errno, cf_strerror(errno));

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
	if (0 > (s->sock = socket(AF_INET, s->proto, 0))) {
		cf_warning(CF_SOCKET, "socket: %s", cf_strerror(errno));
		return(errno);
	}
	s->saddr.sin_family = AF_INET;
	if (1 != inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr)) {
        cf_warning(CF_SOCKET, "inet_pton: %s", cf_strerror(errno));
        return(errno);
	}
    s->saddr.sin_port = htons(s->port);

	if (s->reuse_addr) {
		int v = 1;
		setsockopt(s->sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v) );
	}

	/* Set close-on-exec */
	fcntl(s->sock, F_SETFD, FD_CLOEXEC);

    // I've tried a little tuning here, doesn't seem terribly important.
    // int flag = (1024 * 32);
	// setsockopt(s->sock, SOL_SOCKET, SO_SNDBUF, &flag, sizeof(flag) );
	// setsockopt(s->sock, SOL_SOCKET, SO_RCVBUF, &flag, sizeof(flag) );
    
    // Nodelay is terribly important, but setting here doesn't seem all that effective. Doesn't
    // seem to effect the accepted file descriptors derived from the listen fd
    // int flag = 1;
	// setsockopt(s->sock, SOL_TCP, TCP_NODELAY, &flag, sizeof(flag) );    
    
	/* Bind to the socket; if we can't, nanosleep() and retry */
	while (0 > (bind(s->sock, (struct sockaddr *)&s->saddr, sizeof(struct sockaddr)))) {
		if (EADDRINUSE != errno) {
			cf_warning(CF_SOCKET, "bind: %s", cf_strerror(errno));
			return(errno);
		}

		cf_warning(CF_SOCKET, "bind: socket in use, waiting (port:%d)",s->port);

		nanosleep(&delay, NULL);
	}

	/* Listen for connections */
	if ((SOCK_STREAM == s->proto) && (0 > listen(s->sock, 512))) {
		cf_warning(CF_SOCKET, "listen: %s", cf_strerror(errno));
		return(errno);
	}

	return(0);
}


#define CONNECT_TIMEOUT 1000

/* cf_socket_init_client
 * Connect a socket to a remote endpoint
 * DOES A BLOCKING CONNECT INLINE - timeout
 */
int
cf_socket_init_client(cf_socket_cfg *s)
{
	cf_assert(s, CF_SOCKET, CF_PROCESS, CF_CRITICAL, "invalid argument");

	if (0 > (s->sock = socket(AF_INET, s->proto, 0))) {
		cf_warning(CF_SOCKET, "socket: %s", cf_strerror(errno));
		return(errno);
	}

	fcntl(s->sock, F_SETFD, FD_CLOEXEC);  /* close on exec */
	fcntl(s->sock, F_SETFL, O_NONBLOCK); /* non-blocking */

    // Try tuning the window: must be done before connect
//    int flag = (1024 * 32);
//	setsockopt(s->sock, SOL_SOCKET, SO_SNDBUF, &flag, sizeof(flag) );
//	setsockopt(s->sock, SOL_SOCKET, SO_RCVBUF, &flag, sizeof(flag) );

        
	memset(&s->saddr,0,sizeof(s->saddr));
	s->saddr.sin_family = AF_INET;
    if (0 >= inet_pton(AF_INET, s->addr, &s->saddr.sin_addr.s_addr)) {
		cf_warning(CF_SOCKET, "inet_pton: %s", cf_strerror(errno));
		close(s->sock);
		return(errno);
	}
	s->saddr.sin_port = htons(s->port);

	int rv = connect(s->sock, (struct sockaddr *)&s->saddr, sizeof(s->saddr));
	cf_debug(CF_SOCKET, "connect: rv %d errno %s",rv,cf_strerror(errno));

	if (rv < 0) {
		int epoll_fd = -1;

		if (errno == EINPROGRESS) {
			cf_clock start = cf_getms();

			if (0 > (epoll_fd = epoll_create(1))) {
				cf_warning(CF_SOCKET, "epoll_create() failed (errno %d: \"%s\")", errno, cf_strerror(errno));
				goto Fail;
			}

			struct epoll_event event;
			memset(&event, 0, sizeof(struct epoll_event));
			event.data.fd = s->sock;
			event.events = EPOLLOUT;

			if (0 > epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s->sock, &event)) {
				cf_warning(CF_SOCKET, "epoll_ctl(ADD) of client socket failed (errno %d: \"%s\")", errno, cf_strerror(errno));
				goto Fail;
			}

			int tries = 0;
			do {
				int nevents = 0;
				int max_events = 1;
				int wait_ms = 1;
				struct epoll_event events[max_events];

				if (0 > (nevents = epoll_wait(epoll_fd, events, max_events, wait_ms))) {
					if (errno == EINTR) {
						cf_debug(CF_SOCKET, "epoll_wait() on client socket encountered EINTR ~~ Retrying!");
						goto Retry;
					} else {
						cf_warning(CF_SOCKET, "epoll_wait() on client socket failed (errno %d: \"%s\") ~~ Failing!", errno, cf_strerror(errno));
						goto Fail;
					}
				} else {
					if (nevents == 0) {
						cf_debug(CF_SOCKET, "epoll_wait() returned no events ~~ Retrying!");
						goto Retry;
					}
					if (nevents != 1) {
						cf_warning(CF_SOCKET, "epoll_wait() returned %d events ~~ only 1 expected, so ignoring others!", nevents);
					}
					if (events[0].data.fd == s->sock) {
						if (events[0].events & EPOLLOUT) {
							cf_debug(CF_SOCKET, "epoll_wait() on client socket ready for write detected ~~ Succeeding!");
						} else {
							// (Note:  ERR and HUP events are automatically waited for as well.)
							if (events[0].events & (EPOLLERR | EPOLLHUP)) {
								cf_warning(CF_SOCKET, "epoll_wait() on client socket detected failure event 0x%x ~~ Failing!", events[0].events);
							} else {
								cf_warning(CF_SOCKET, "epoll_wait() on client socket detected non-write events 0x%x ~~ Failing!", events[0].events);
							}
							goto Fail;
						}
					} else {
						cf_warning(CF_SOCKET, "epoll_wait() on client socket returned event on unknown socket %d ~~ Retrying!", events[0].data.fd);
						goto Retry;
					}
					if (0 > epoll_ctl(epoll_fd, EPOLL_CTL_DEL, s->sock, &event)) {
						cf_warning(CF_SOCKET, "epoll_ctl(DEL) on client socket failed (errno %d: \"%s\")", errno, cf_strerror(errno));
					}
					close(epoll_fd);
					goto Success;
				}
Retry:
				cf_debug(CF_SOCKET, "Connect epoll loop:  Retry #%d", tries++);
				if (start + CONNECT_TIMEOUT < cf_getms()) {
					cf_warning(CF_SOCKET, "Error in delayed connect(): timed out");
					errno = ETIMEDOUT;
					goto Fail;
				}
			} while (1);
		}
Fail:
		cf_debug(CF_SOCKET, "connect fail: %s", cf_strerror(errno));


		if (epoll_fd > 0) {
			close(epoll_fd);
		}

		close(s->sock);
		s->sock = -1;
		return(errno);
	} else {
		cf_debug(CF_SOCKET, "client socket connect() in 1 try!");
	}
Success:	;

    // regarding this: calling here doesn't seem terribly effective.
    // on the fabric threads, it seems important to set nodelay much later
    int flag = 1;
	setsockopt(s->sock, SOL_TCP, TCP_NODELAY, &flag, sizeof(flag));
	long farg = fcntl(s->sock, F_GETFL, 0);
	fcntl(s->sock, F_SETFL, farg & (~O_NONBLOCK)); /* blocking again */
    
	return(0);
}

/* cf_socket_close
 * Close a socket originally opened listening
 */
void
cf_socket_close(cf_socket_cfg *s)
{
	if (!s) {
		cf_warning(CF_SOCKET, "not closing null socket!");
		return;
	}
	
	shutdown(s->sock, SHUT_RDWR);
	close(s->sock);
	s->sock = -1;
}

/* cf_socket_connect_nb
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
    if (ms->tx_addr) {
        struct in_addr iface_in;
        memset((char *)&iface_in,0,sizeof(iface_in));
        iface_in.s_addr = inet_addr(ms->tx_addr);

        if(setsockopt(s->sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&iface_in, sizeof(iface_in)) == -1) {
            cf_warning(CF_SOCKET, "IP_MULTICAST_IF: %d %s", errno, cf_strerror(errno));
    		return(errno);
        }
    }
    unsigned char ttlvar = ms->mcast_ttl;
    if (ttlvar>0) {
		if (setsockopt(s->sock,IPPROTO_IP,IP_MULTICAST_TTL,(char *)&ttlvar,
				sizeof(ttlvar)) == -1) {
			cf_warning(CF_SOCKET, "IP_MULTICAST_TTL: %d %s", errno, cf_strerror(errno));
		} else {
			cf_info(CF_SOCKET, "setting multicast TTL to be %d",ttlvar);
		}
    }
	while (0 > (bind(s->sock, (struct sockaddr *)&s->saddr, sizeof(struct sockaddr)))) {
		cf_info(CF_SOCKET, "multicast socket bind failed: %d %s", errno, cf_strerror(errno));
	}

	// Register for the multicast group
	inet_pton(AF_INET, s->addr, &ms->ireq.imr_multiaddr.s_addr);
	ms->ireq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (ms->tx_addr) {
    	ms->ireq.imr_interface.s_addr = inet_addr(ms->tx_addr);
    }
	setsockopt(s->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&ms->ireq, sizeof(struct ip_mreq));

	return(0);
}


void
cf_mcastsocket_close(cf_mcastsocket_cfg *ms)
{
	cf_socket_cfg *s = &(ms->s);

	close(s->sock);
}

//
// get information about the interfaces and what their addresses are
//

// Pass in a buffer that you think is big enough, and it'll get filled out
// error will return if you haven't passed in enough data
// ordering not guarenteed

int
cf_ifaddr_get( cf_ifaddr **ifaddr, int *ifaddr_sz, uint8_t *buf, size_t bufsz)
{
	struct ifaddrs *ifa;
	int rv = getifaddrs(&ifa);
	if (rv != 0) {
		cf_info(CF_SOCKET, " could not get interface information: return value %d errno %d",rv,errno);
		return(-1);
	}
	struct ifaddrs *ifa_orig = ifa;
	
	// currently, return ipv4 only (?)
	int n_ifs = 0;
	while (ifa) {
		if ((ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_INET)) {
			n_ifs++;
		}
		ifa = ifa->ifa_next;
	}
	
	if (bufsz < sizeof(cf_ifaddr) * n_ifs) {
		freeifaddrs(ifa_orig);
		return(-2);
	}
	
	*ifaddr_sz = n_ifs;
	*ifaddr = (cf_ifaddr *) buf;
	ifa = ifa_orig;
	int i = 0;
	while (ifa) {
		
		if ((ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_INET))
		{
		
			(*ifaddr)[i].flags = ifa->ifa_flags;
			(*ifaddr)[i].family = ifa->ifa_addr->sa_family;
			memcpy( &((*ifaddr)[i].sa), ifa->ifa_addr, sizeof(struct sockaddr) ); 
			
			i++;
		}
		ifa = ifa->ifa_next;
	}

	freeifaddrs(ifa_orig);
	return(0);
}

int
cf_socket_test()
{
	fd_set fdset;
	int i, max_fds = 2048, fd[max_fds], max_fd = 0;

	cf_info(CF_SOCKET, "Performing cf_socket tests:");

#ifdef _FORTIFY_SOURCE
	cf_info(CF_SOCKET, "_FORTIFY_SOURCE = %d", _FORTIFY_SOURCE);
#else
	cf_info(CF_SOCKET, "_FORTIFY_SOURCE undefined!");
#endif

#ifdef __USE_FORTIFY_LEVEL 
	cf_info(CF_SOCKET, "__USE_FORTIFY_LEVEL = %d", __USE_FORTIFY_LEVEL);
#else
	cf_info(CF_SOCKET, "_USE_FORTIFY_LEVEL undefined!");
#endif

	cf_info(CF_SOCKET, "max_fds = %d ; sizeof(fd_set) = %lu ; FD_SETSIZE = %d", max_fds, sizeof(fd_set), FD_SETSIZE);

	FD_ZERO(&fdset);

	for (i = 0; i < max_fds; i++) {
		if (!(fd[max_fd = i] = socket(AF_INET, SOCK_STREAM, PF_INET))) {
			cf_info(CF_SOCKET, "socket() returned 0 @ #%d", i);
			break;
		}
		FD_SET(fd[i], &fdset);
	}

	cf_info(CF_SOCKET, "Opened %d sockets!", max_fd + 1);

	for (i = 0; i < max_fd; i++) {
		FD_CLR(fd[i], &fdset);
		close(fd[i]);
	}

	cf_info(CF_SOCKET, "Closed %d sockets!", max_fd + 1);

	cf_info(CF_SOCKET, "cf_socket tests succeeded.");

	return 0;
}
