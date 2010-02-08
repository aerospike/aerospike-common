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


uint32_t
cf_nodeid_shash_fn(void *value)
{
	uint32_t *b = value;
	uint32_t acc = 0;
	for (int i=0;i<sizeof(cf_node);i++) {
		acc += *b;
	}
	return(acc);
}

uint32_t
cf_nodeid_rchash_fn(void *value, uint32_t value_len)
{
	uint32_t *b = value;
	uint32_t acc = 0;
	for (int i=0;i<sizeof(cf_node);i++) {
		acc += *b;
	}
	return(acc);
}

/*
 * Gets the ip address of an interface.
 */

int
cf_ipaddr_get(int socket, char *nic_id, char **node_ip )
{
	struct sockaddr_in sin;
	struct ifreq ifr;
	in_addr_t ip_addr;
	
	memset(&ip_addr, 0, sizeof(in_addr_t));
	memset(&sin, 0, sizeof(struct sockaddr));
	memset(&ifr, 0, sizeof(ifr));
	
	// copy the nic name (eth0, eth1, eth2, etc.) ifr variable structure
	strncpy(ifr.ifr_name, nic_id, IFNAMSIZ);
	
	// get the ifindex for the adapter...
	if (ioctl(socket, SIOCGIFINDEX, &ifr) < 0) {
		cf_debug(CF_MISC, "Can't get ifindex for adapter %s - %d %s\n", nic_id, errno, cf_strerror(errno));
		return(-1);
	}
	
	// get the IP address
	memset(&sin, 0, sizeof(struct sockaddr));
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, nic_id, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;
	if (ioctl(socket, SIOCGIFADDR, &ifr)< 0)    {
		cf_debug(CF_MISC, "can't get IP address: %d %s", errno, cf_strerror(errno));
		return(-1);
	}
	memcpy(&sin, &ifr.ifr_addr, sizeof(struct sockaddr));
	ip_addr = sin.sin_addr.s_addr;
	char cpaddr[24];
	if (NULL == inet_ntop(AF_INET, &ip_addr, (char *)cpaddr, sizeof(cpaddr))) {
		cf_warning(CF_MISC, "received suspicious address %s : %s", cpaddr, cf_strerror(errno));
		return(-1);
	}
	cf_info (CF_MISC, "Node ip: %s", cpaddr);
	*node_ip = strdup(cpaddr);

	return(0);
}

/*
 * Gets a unique id for this process instance
 * Uses the mac address right now
 * And combine with the unique port number, which is why it needs to be passed in
 *   (kind of grody ass shit)
 * Needs to be a little more subtle:
 * Should stash the mac address or something, in case you have to replace a card.
 */

// names to check, in order
//
char *interface_names[] = { "eth%d", "bond%d", 0 };

int
cf_nodeid_get( unsigned short port, cf_node *id, char **node_ipp, hb_mode_enum hb_mode, char **hb_addrp )
{

	int fdesc;
	struct ifreq req;

	if (0 >= (fdesc = socket(AF_INET, SOCK_STREAM, 0))) {
		cf_warning(CF_MISC, "can't open socket: %d %s", errno, cf_strerror(errno));
		return(-1);
	}
	int i = 0;
	bool done = false;
	
	while ((interface_names[i]) && (done == false)) {
		
		int j=0;
		while ( (done == false) && (j < 11) ) {
	
			sprintf(req.ifr_name, interface_names[i],j);
	
			if (0 == ioctl(fdesc, SIOCGIFHWADDR, &req)) { 
				if (cf_ipaddr_get(fdesc, req.ifr_name, node_ipp) == 0) {
					done = true;
					break;
				}
			}
	
			cf_debug(CF_MISC, "can't get physical address of interface %s: %d %s", req.ifr_name, errno, cf_strerror(errno));
	
			j++;
	
		}
		i++;
	}

	if (done == false) {
		cf_warning(CF_MISC, "can't get physical address, tried eth and bond, fatal: %d %s", errno, cf_strerror(errno));
		close(fdesc);
		return(-1);
		
	}
	
    close(fdesc);
    /*
     * Set the hb_addr to be the same as the ip address if the mode is mesh and the hb_addr parameter is empty
     * Configuration file overrides the automatic node ip detection
     *    - this gives us a work around in case the node ip is somehow detected wrong in production
     */
     if (hb_mode == AS_HB_MODE_MESH)
     {
     	if (*hb_addrp == NULL)
     		*hb_addrp = strdup(*node_ipp);
     		
        cf_info (CF_MISC, "Heartbeat address for mesh: %s", *hb_addrp);		
     }
        	
	*id = 0;
	memcpy(id, req.ifr_hwaddr.sa_data, 6);
	
	memcpy( ((byte *)id) + 6, &port, 2);

	cf_debug(CF_MISC, "port %d id %"PRIx64, port, *id);

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

