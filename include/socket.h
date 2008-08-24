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


/* cf_socket_cfg
 * A socket, which can be used for either inbound or outbound connections */
struct cf_socket_cfg_t {
	char *addr;
	int port;

	int sock;
	struct sockaddr_in saddr;
};
typedef struct cf_socket_cfg_t cf_socket_cfg;

/* cf_mcastsocket_cfg
 * A multicast socket */
struct cf_mcastsocket_cfg_t {
	cf_socket_cfg socket;
	struct ip_mreq ireq;
};
typedef struct cf_mcastsocket_cfg_t cf_mcastsocket_cfg;

/* Function declarations */
extern int cf_socket_recv(int s, void *buf, size_t buflen, int flags);
extern int cf_socket_send(int s, void *buf, size_t buflen, int flags);
extern int cf_socket_init_svc(cf_socket_cfg *s);
extern int cf_socket_init_client(cf_socket_cfg *s);
extern void cf_mcastsocket_init(cf_mcastsocket_cfg *s);
