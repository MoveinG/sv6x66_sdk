/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 *
 */
#ifndef __AYLA_PFM_COMMON_NET_TLS_H__
#define __AYLA_PFM_COMMON_NET_TLS_H__

#include <ayla/utypes.h>
#include <al/al_err.h>
#include <platform/pfm_net_tcp.h>

/**
 * @file
 * Network TLS interfaces
 */

/**
 * Structure of TLS control block.
 */
struct pfm_net_tls;

/**
 * Initialize TLS (Transport Layer Security).
 */
void pfm_net_tls_init(void);

/**
 * Finalize TLS.
 */
void pfm_net_tls_final(void);

/**
 * Connect to the remote server. Verify the server's certificate and
 * negotiate symmetric key.
 *
 * \param fd is a socket that is connected to the server.
 * \param hostname is the remote host name. Is is required on certificates
 * checking.
 * \param timeout is the timeout for handshaking in ms.
 *
 * \returns a pointer to struct pfm_net_tls, or NULL on failure.
 */
struct pfm_net_tls *pfm_net_tls_handshake(int fd,
	const char *hostname, int timeout);

/**
 * Send data to the remote server.
 *
 * \param tls is a pointer to struct pfm_net_tls.
 * \param data is the data to be sent.
 * \param size is the length of the data to be sent.
 *
 * \returns bytes that actually sent.
 */
int  pfm_net_tls_write(struct pfm_net_tls *tls, const void *data, int size);

/**
 * Receive data from socket. And then decode the data, and return it to
 * the caller.
 *
 * \param tls is a pointer to struct pfm_net_tls.
 * \param buf is the buffer for decoded data.
 * \param size is the buffer size.
 *
 * \returns bytes received.
 */
int  pfm_net_tls_read(struct pfm_net_tls *tls, void *buf, int size);

/**
 * Free the TLS control block returned by pfm_tls_handshake().
 *
 * \param tls is the pointer to struct pfm_net_tls that to be freed.
 *
 */
void pfm_net_tls_free(struct pfm_net_tls *tls);

/**
 * Get ssl link status. It is used to check if the security link is
 * is timeout (so disconnected by the server) or not.
 *
 * \param tls is a pointer to struct pfm_net_tls.
 *
 * \returns zero on OK.
 */
int pfm_net_tls_get_link_status(struct pfm_net_tls *tls);

#endif /* __AYLA_PFM_COMMON_NET_TLS_H__ */
