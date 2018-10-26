/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_NET_UDP_H__
#define __AYLA_AL_COMMON_NET_UDP_H__

#include <al/al_utypes.h>
#include <al/al_err.h>
#include <al/al_net_addr.h>

/**
 * @file
 * Platform Network UDP Interfaces
 *
 * TBD, need text for overall considerations of threading and locking, etc.
 */

/*
 * References to structures defined elsewhere.
 */
struct al_net_if;

/**
 * Opaque structure for the UDP protocol control block (PCB).
 */
struct al_net_udp;

/**
 * Allocate a UDP PCB.
 *
 * This allocates and returns a UDP PCB.
 * \returns the UDP PCB or NULL if the allocation fails.
 */
struct al_net_udp *al_net_udp_new(void);

/**
 * Bind a UDP PCB to a local IP address and port.
 *
 * \param udp is the UDP PCB.
 * \param addr is the local address to use, or NULL if any address can be used.
 * \param port is the UDP port number, in host order.
 * \returns zero on success, error number on error.
 */
enum al_err al_net_udp_bind(struct al_net_udp *udp,
		struct al_net_addr *addr, u16 port);

/**
 * Set the remote address and/or port for a UDP PCB PCB.
 *
 * This sets the destination address for subsequent al_net_udp_send() calls.
 *
 * \param udp is the UDP PCB.
 * \param addr is the remote address to use.
 * \param port is the remote UDP port number, in host order.
 * \returns zero on success, error number on error.
 */
enum al_err al_net_udp_connect(struct al_net_udp *udp,
		struct al_net_addr *addr, u16 port);

/**
 * Set the recveive callback for a UDP PCB.
 *
 * \param udp is the UDP PCB.
 * \param recv_cb is a function that will be called when a packet is received
 * on the UDP PCB.
 * \param recv_arg is the value to be passed as the first argument to the
 * receive callback.
 *
 * The arguments of recv_cb:
 * arg: It is the last paramter of al_net_udp_set_recv_cb().
 * udp: It is the net-udp object which reports the received data.
 * data: UDP packet received. It is allocated in net-udp. The upper layer 
 * (or data receiver) should use al_net_udp_buf_free() to free the memory.
 * len: It is the legnth of the received data.
 * from_ip: It is the source IP address that the packet comes from.
 * from_port: It is the source port, in host order.
 * net_if: It is the network interface pointer, or NULL if unknown.
 */
void al_net_udp_set_recv_cb(struct al_net_udp *udp,
		void (*recv_cb)(void *arg, struct al_net_udp *udp,
		void *data, size_t len,
		struct al_net_addr *from_ip, unsigned short from_port,
		struct al_net_if *net_if), void *recv_arg);

/**
 * Allocate a UDP packet buffer.
 *
 * \param len is the size of the buffer in bytes.
 * \returns a pointer to data buffer on success, NULL on error.
 *
 * Usage: 
 *   For sending, upper layer uses al_net_udp_buf_alloc() to allocate
 * a packet buffer. The net-udp layer will use al_net_udp_buf_free()
 * to free it when it is sent out.
 *   For receiving, net-udp layer uses al_net_udp_buf_alloc() to alocate
 * a packet buffer. The upper layer should use al_net_udp_buf_free() to
 * free it.
 */
void *al_net_udp_buf_alloc(size_t len);

/**
 * Free UDP packet buffer.
 *
 * \param buf is a pointer to data buffer. If NULL, it is ignored.
 */
void al_net_udp_buf_free(void *buf);

/**
 * Send a packet on the UDP PCB.
 *
 * \param udp is the UDP PCB.
 * \param buf is a pointer to the data to be sent. The caller should use 
 * al_net_udp_buf_alloc() to allocate the data buffer. It will be freed
 * by net-udp when the packet has been sent out.
 * \param len is length of the data to be sent.
 *
 * \returns zero on success, error number on error.
 *
 * The UDP buffer is freed, even on error.
 */
enum al_err al_net_udp_send(struct al_net_udp *udp, void *buf,
	size_t len);

/**
 * Sent a packet on the UDP PCB.
 *
 * \param udp is the UDP PCB.
 * \param buf is a pointer to the data to be sent. The caller should use
 * al_net_udp_buf_alloc() to allocate the buffer. It will be freed by
 * net-udp when it has been sent out.
 * \param len is the length of the data to be sent.
 * \param to is the IP address of the destination.
 * \param port is the UDP destination port in host order.
 * \param nif is the network interface to use. If nif is NULL,
 * any interface with a route to the destination may be used.
 * \returns zero on success, error number on error.
 *
 * The UDP buffer is freed, even on error.
 */
enum al_err al_net_udp_sendto_if(struct al_net_udp *udp,
		void *buf, size_t len,
		struct al_net_addr *to, unsigned short port,
		struct al_net_if *nif);

/**
 * Free a UDP PCB.
 *
 * Frees the UDP PCB, cancelling any pending callbacks.
 * \param udp is the UDP PCB.
 */
void al_net_udp_free(struct al_net_udp *udp);

/**
 * Join a UDP multicast group.
 *
 * \param udp is the UDP PCB.
 * \param if_addr is the network interface IP address.
 * \param group is the IP multicast group address.
 * \returns zero on success, error number on error.
 */
enum al_err al_net_igmp_joingroup(struct al_net_udp *udp,
		struct al_net_addr *if_addr,
		struct al_net_addr *group);

/**
 * Leave a UDP multicast group.
 *
 * \param udp is the UDP PCB.
 * \param if_addr is the network interface IP address.
 * \param group is the IP multicast group address.
 * \returns zero on success, error number on error.
 */
enum al_err al_net_igmp_leavegroup(struct al_net_udp *udp,
		struct al_net_addr *if_addr,
		struct al_net_addr *group);

#endif /* __AYLA_AL_COMMON_NET_UDP_H__ */
