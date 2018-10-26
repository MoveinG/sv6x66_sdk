/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_NET_IF_H__
#define __AYLA_AL_COMMON_NET_IF_H__

#include <al/al_utypes.h>
#include <al/al_net_addr.h>

/**
 * @file
 * Platform Network Interface APIs
 */

/**
 * Opaque network interface structure.
 */
struct al_net_if;

/**
 * Network interface type.
 */
enum al_net_if_type {
	AL_NET_IF_DEF,	/**< Default interface */
	AL_NET_IF_STA,	/**< Wifi station interface */
	AL_NET_IF_AP,	/**< Wifi AP interface */
	AL_NET_IF_MAX,	/**< Wifi AP interface limit */
};

/**
 * Get network interface.
 *
 * \param type specifies the network interface.
 *
 * \return the network interface, NULL is for no the interface.
 */
struct al_net_if *al_get_net_if(enum al_net_if_type type);

/**
 * Get IPv4 address is associated with the network interface
 *
 * \param net_if is a pointer to network interface structure.
 *
 * \return the IPv4 address in host byte order.
 */
u32 al_net_if_get_ipv4(struct al_net_if *net_if);

/**
 * Get netmask is associated with the network interface
 *
 * \param net_if is a pointer to network interface structure.
 *
 * \return the metmask in host byte order.
 */
u32 al_net_if_get_netmask(struct al_net_if *net_if);

/**
 * Get MAC address is associated with the network interface
 *
 * \param net_if is a pointer to network interface structure.
 * \param mac_addr points a buffer to retrieve the MAC address.
 *
 * \return zero on success, -1 on error.
 */
int al_net_if_get_mac_addr(struct al_net_if *net_if, u8 mac_addr[6]);

/**
 * Get network address is associated with the network interface
 *
 * \param net_if is a pointer to network interface structure.
 *
 * \return the network address structure pointer.
 */
struct al_net_addr *al_net_if_get_addr(struct al_net_if *net_if);

#endif /* __AYLA_AL_COMMON_NET_IF_H__ */
