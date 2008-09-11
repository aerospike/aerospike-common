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
 * Gets a unique id for this machine
 * Uses the mac address right now
 * Needs to be a little more subtle:
 * Should stash the mac address or something, in case you have to replace a card.
 */


#define pNIC "eth0"

int
cf_id_get( uint64_t *id )
{

	int fdesc;
	struct ifreq req;
	int err;

	if (0 >= (fdesc = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP))) {
		return(-1);
	}

	strcpy(req.ifr_name, pNIC);
	if (0 > (err = ioctl(fdesc, SIOCGIFHWADDR, &req))) {
		close(fdesc);
		return(-1);
	}

	close(fdesc);
	*id = 0;
	memcpy(id, req.ifr_hwaddr.sa_data, 6);

	return(0);
}

