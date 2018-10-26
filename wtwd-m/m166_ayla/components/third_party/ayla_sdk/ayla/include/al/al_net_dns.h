/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_NET_DNS_H__
#define __AYLA_NET_DNS_H__

#include <al/al_utypes.h>
#include <al/al_err.h>
#include <al/al_net_addr.h>

/**
 * @file
 * Platform Network DNS Interfaces
 */

/**
 * Structure representing a DNS request.
 *
 * Note that this does not handle IPv6 at this point.
 * The requester should allocate this so that it persists for the duration
 * of the DNS request.  Zero it and then fill in the hostname and callback
 * function.
 */
struct al_net_dns_req {
	const char *hostname;	/*!< hostname being looked up */
	enum al_err error;	/*!< result error code (or in-progress) */
	void (*callback)(struct al_net_dns_req *);
				/*!< function for delivering result */
	struct al_net_addr addr;/*!< result of network address */
	void *al_priv;		/*!< private pointer for adaptation layer */
};

/**
 * Request an IPv4 DNS lookup by hostname.
 *
 * \param req the DNS request structure to use, possibly uninitialized.
 *
 * The callback is always made asynchronously in the main thread.
 * The request may be cancelled.And the req should be valid before the
 * al_dns_req_cancel is called.
 * This call must not block.
 *
 * The callback must be done in the same thread and may
 * be synchronous (on error or cached IP result), but otherwise will be
 * asynchronous.  TBD: Since the callback must be in the same thread,
 * this is OK, but if we used multiple threads, the callback would
 * need to always asynch.
 * \returns zero on success, possible error codes are TBD.
 */
enum al_err al_dns_req_ipv4_start(struct al_net_dns_req *req);

/**
 * Cancel an DNS request.
 *
 * \param req the DNS request.
 */
void al_dns_req_cancel(struct al_net_dns_req *req);

/**
 * Delete a host from the DNS lookup cache.
 *
 * \param hostname the hostname.
 */
void al_net_dns_delete_host(const char *hostname);

/**
 * Switch to the next DNS server in sequence, if possible.
 * Used in error recovery.
 */
void al_net_dns_servers_rotate(void);

#endif /* __AYLA_NET_DNS_H__ */
