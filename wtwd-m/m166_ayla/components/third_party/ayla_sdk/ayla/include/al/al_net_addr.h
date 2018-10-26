/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_NET_ADDR_H__
#define __AYLA_AL_COMMON_NET_ADDR_H__

#include <al/al_utypes.h>
#include <platform/pfm_net_addr.h>

/**
 * @file
 * Platform Network Address Interfaces
 */

/**
 * Opaque network address structure.
 *
 * The structure is platform depend, it is defined in the file
 * platform/pfm_net_addr.h.
 */
struct al_net_addr;

/**
 * Get IPv4 address from the network address structure.
 *
 * \param addr is a pointer to network address structure.
 *
 * \return the IPv4 address in host byte order.
 */
u32 al_net_addr_get_ipv4(const struct al_net_addr *addr);

/**
 * Set IPv4 address into the network address structure
 *
 * \param addr is a pointer to network address structure.
 * \param ip is the IPv4 address in host byte order
 */
void al_net_addr_set_ipv4(struct al_net_addr *addr, u32 ip);

#endif /* __AYLA_AL_COMMON_NET_ADDR_H__ */
