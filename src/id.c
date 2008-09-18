/*
 *  Citrusleaf Foundation
 *  src/socket.c - socket functions
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include "cf.h"

/*
 * Gets a unique id for this process instance
 * Uses the mac address right now
 * And combine with the unique port number, which is why it needs to be passed in
 *   (kind of grody ass shit)
 * Needs to be a little more subtle:
 * Should stash the mac address or something, in case you have to replace a card.
 */



int
cf_nodeid_get( unsigned short port, cf_node *id )
{

	int fdesc;
	struct ifreq req;

	if (0 >= (fdesc = socket(AF_INET, SOCK_STREAM, 0))) {
		D("cf_id_get: can't open socket error %d %s",errno, cf_strerror(errno));
		return(-1);
	}
	int i;
	for (i=0;i<10;i++) {
		sprintf(req.ifr_name, "eth%d",i);
		if (0 == ioctl(fdesc, SIOCGIFHWADDR, &req)) 
			break;
		D("cf_id_get: can't get mac id eth%d %d %s",i,errno, cf_strerror(errno));
	}
	close(fdesc);
	if (i == 10) {
		D("cf_id_get: can't get mac id %d %s",errno, strerror(errno));
		return(-1);
	}

	*id = 0;
	memcpy(id, req.ifr_hwaddr.sa_data, 6);
	
	memcpy( ((byte *)id) + 6, &port, 2);

	return(0);
}

