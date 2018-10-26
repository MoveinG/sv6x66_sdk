/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_net_if.h>
#include <soc_types.h>

#define INTERFACE_NAME "et0"

struct al_net_if{
	unsigned char interface[16];
};
static struct al_net_if g_net_if;

struct al_net_if *al_get_net_if(enum al_net_if_type type)
{
	strcpy(g_net_if.interface, INTERFACE_NAME);
	return &g_net_if;
}

u32 al_net_if_get_ipv4(struct al_net_if *net_if)
{
	unsigned char dhcpen;
    u8 mac[6];
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;
	get_if_config_2(INTERFACE_NAME, &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
	return (ipaddr.u16[0] << 16+ipaddr.u16[1]);
}

u32 al_net_if_get_netmask(struct al_net_if *net_if)
{
	unsigned char dhcpen;
    u8 mac[6];
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;
	get_if_config_2(INTERFACE_NAME, &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
	return (submask.u16[0] << 16+submask.u16[1]);
}

int al_net_if_get_mac_addr(struct al_net_if *net_if, u8 mac_addr[6])
{
	unsigned char dhcpen;
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;
	get_if_config_2(INTERFACE_NAME, &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac_addr, 6);	
	return 0;
}

struct al_net_addr *al_net_if_get_addr(struct al_net_if *net_if)
{
	return NULL;
}
