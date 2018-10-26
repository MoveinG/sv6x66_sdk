/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_PFM_COMMON_NET_DHCPS_H__
#define __AYLA_PFM_COMMON_NET_DHCPS_H__

#include <al/al_net_if.h>


struct pfm_dhcps_srv_arg {
	u32 local_ip;
	struct al_net_if *netif;
	u32 ip_pool_start;
	u32 ip_pool_end;
	u32 ip_pool_mask;
	u32 default_gw;
	u32 dns_server1;
	u32 dns_server2;
};

/**
 * Start DHCP server.
 *
 * \param local_addr is the local IP address that socket will bind to.
 * \param local_if is net interface.
 *
 * \return status code. 0 is success.
 */
int pfm_dhcps_server_start(struct pfm_dhcps_srv_arg *arg);

/**
 * Stop DHCP server.
 *
 */
void pfm_dhcps_server_stop(void);

#endif /* __AYLA_PFM_COMMON_NET_TCP_H__ */
