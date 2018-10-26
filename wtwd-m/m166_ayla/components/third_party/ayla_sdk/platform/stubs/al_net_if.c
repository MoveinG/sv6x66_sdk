/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_net_if.h>

struct al_net_if *al_get_net_if(enum al_net_if_type type)
{
	return NULL;
}

u32 al_net_if_get_ipv4(struct al_net_if *net_if)
{
	return 0;
}

u32 al_net_if_get_netmask(struct al_net_if *net_if)
{
	return 0;
}

int al_net_if_get_mac_addr(struct al_net_if *net_if, u8 mac_addr[6])
{
	return 0;
}

struct al_net_addr *al_net_if_get_addr(struct al_net_if *net_if)
{
	return NULL;
}
