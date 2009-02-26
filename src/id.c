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
** need a spot for tihs
*/

cf_digest cf_digest_zero = { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } };

/*
** nodeids are great things to use as keys in the hash table 
*/


uint32
cf_nodeid_hash_fn(void *value)
{
	byte *b = value;
	uint32 acc = 0;
	for (int i=0;i<sizeof(cf_node);i++) {
		acc += *b;
	}
	return(acc);
}

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

	D("cf_nodeid_get: port %d result %"PRIx64,port,*id);
	
	return(0);
}

/*
** if I receive one of these encoded node_ids, I might want to pluck out the port
*/
unsigned short
cf_nodeid_get_port(cf_node id)
{
	byte *b = (byte *) &id;
	unsigned short port;
	memcpy(&port, &b[6], 2);
	return(port);
	
}

