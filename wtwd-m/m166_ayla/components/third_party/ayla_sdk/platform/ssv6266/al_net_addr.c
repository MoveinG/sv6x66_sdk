/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <al/al_net_addr.h>
#include <ayla/assert.h>
#include "lwip/inet.h"

u32 al_net_addr_get_ipv4(const struct al_net_addr *addr)
{
	u32 ip;

	ASSERT(NULL != addr);

	ip = (u32)ntohl((u32)addr->ip);
	return ip;
}

void al_net_addr_set_ipv4(struct al_net_addr *addr, u32 ip)
{
	ASSERT(NULL != addr);

	addr->ip = htonl(ip);
}
