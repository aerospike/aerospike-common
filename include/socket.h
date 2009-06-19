/*
 *  Citrusleaf Foundation
 *  include/socket.h - socket structures
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


/* SYNOPSIS
 * */

// the reality is all addresses are IPv4, even with the coming ipv6, an address can
// fit easily in a uint64_t. Create a type and some utility routines so you can just
// traffic in an address type.

typedef uint64_t cf_sockaddr;

/* cf_socket_cfg
 * A socket, which can be used for either inbound or outbound connections */
struct cf_socket_cfg_t {
	char *addr;
	int port;
	bool reuse_addr; // set if you want 'reuseaddr' for server socket setup
					 // not recommended for production use, rather nice for debugging
	int proto;

	int sock;
	struct sockaddr_in saddr;
};
typedef struct cf_socket_cfg_t cf_socket_cfg;

/* cf_mcastsocket_cfg
 * A multicast socket */
struct cf_mcastsocket_cfg_t {
	
	cf_socket_cfg s;
	struct ip_mreq ireq;
};
typedef struct cf_mcastsocket_cfg_t cf_mcastsocket_cfg;

/* Function declarations */
extern int cf_socket_set_nonblocking(int s);
extern int cf_socket_recv(int sock, void *buf, size_t buflen, int flags);
extern int cf_socket_send(int sock, void *buf, size_t buflen, int flags);
extern int cf_socket_init_svc(cf_socket_cfg *s);
extern int cf_socket_init_client(cf_socket_cfg *s);
extern int cf_mcastsocket_init(cf_mcastsocket_cfg *ms);
extern void cf_mcastsocket_close(cf_mcastsocket_cfg *ms);
extern int cf_socket_recvfrom(int sock, void *buf, size_t buflen, int flags, cf_sockaddr *from);
extern int cf_socket_sendto(int sock, void *buf, size_t buflen, int flags, cf_sockaddr to);

extern int cf_socket_connect_nb(cf_sockaddr so, int *fd);
extern void cf_sockaddr_convertto(const struct sockaddr_in *src, cf_sockaddr *dst);
extern void cf_sockaddr_convertfrom(const cf_sockaddr src, struct sockaddr_in *dst);
extern void cf_sockaddr_setport(cf_sockaddr *so, unsigned short port);

/*
** get information about all interfaces
** currently returns ipv4 only - but does return loopback
**
** example:
**
** uint8_t buf[512];
** cf_ifaddr *ifaddr;
** int        ifaddr_sz;
** cf_ifaddr_get(&ifaddr, &ifaddr_sz, buf, sizeof(buf));
** 
*/

typedef struct cf_ifaddr_s {
	uint32_t		flags;
	unsigned short	family;
	struct sockaddr sa;
} cf_ifaddr;	

extern int cf_ifaddr_get(cf_ifaddr **ifaddr, int *ifaddr_sz, uint8_t *buf, size_t buf_sz);

