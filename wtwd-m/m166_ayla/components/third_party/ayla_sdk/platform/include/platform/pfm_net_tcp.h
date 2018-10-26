/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_PFM_COMMON_NET_TCP_H__
#define __AYLA_PFM_COMMON_NET_TCP_H__

#include <al/al_err.h>
#include <al/al_net_addr.h>

/**
 * @file
 * Platform Network TCP Interfaces
 *
 * TBD, need text for overall considerations of threading and locking, etc.
 */

struct al_net_if;

/**
 * Opaque structure representing a TCP connection.
 */
struct pfm_net_tcp;

/**
 * Allocate a TCP protocol control block (PCB).
 *
 * This allocates and returns a TCP PCB.
 * \returns a pointer to the PCB, or NULL if the allocation fails.
 */
struct pfm_net_tcp *pfm_net_tcp_new(void);

/**
 * Bind a TCP PCB to a local IP address and port.
 *
 * This binds the TCP PCB to a local IP address and port.
 *
 * \param tcp is the TCP PCB.
 * \param addr is the IPv4 or IPv6 local address, or NULL if
 * any address can be used.
 * \param port is the TCP port number, in host order.
 * \returns zero on success, possible error codes are TBD.
 */
enum al_err pfm_net_tcp_bind(struct pfm_net_tcp *tcp,
		struct al_net_addr *addr, unsigned short port);

/**
 * Set a TCP PCB to listen for incoming connections.
 *
 * Up to backlog unaccepted connections are queued on the PCB.
 *
 * \param tcp is the TCP PCB.
 * \param backlog is the limit on how many waiting connections are allowed.
 * \returns the TCP PCB for listening, which may or may not be
 * the same one passed in.
 *
 * [TBD: perhaps this should be simplified to give the port and
 * network interface only?]
 */
struct pfm_net_tcp *pfm_net_tcp_listen(struct pfm_net_tcp *tcp, int backlog);

/**
 * Set the accept callback function for a listening TCP PCB.
 *
 * The accept callback will be called whenever a new connection arrives.
 *
 * \param tcp is the TCP PCB.
 * \param accepted_cb is the callback function to be called in the
 * event of an incoming connection.
 *
 * The accept callback first argument is the value set by pfm_net_tcp_set_arg(),
 * the second argument is the new TCP PCB for the incoming connection, the
 * final argument is an error code which will be zero unless an error occurs.
 * If the accept callback returns non-zero, the connection is rejected.
 * TBD: permissible error codes.
 */
void pfm_net_tcp_accept(struct pfm_net_tcp *tcp,
		enum al_err (*accepted_cb)(void *arg,
		    struct pfm_net_tcp *new_tcp,
		enum al_err err));

/**
 * Start a connection using the TCP PCB to the remote address and port.
 *
 * Once the connection is established, call the function "connected".
 *
 * \param tcp is the TCP PCB.
 * \param connected_cb is the callback function to be called when the
 * connection completes or the attempt is terminated by an error.
 * \param addr is the remote IP address.
 * \param port is the remote TCP port, in host order.
 * \returns zero on success, error code on failure.
 *
 * The first argument connected-callback is the value set by
 * pfm_net_tcp_set_arg(). The second argument is the TCP PCB. The final
 * argument is an error code which will be zero unless an error occurs.
 * TBD: possible error codes.
 */
enum al_err pfm_net_tcp_connect(struct pfm_net_tcp *tcp,
		struct al_net_addr *addr, unsigned short port,
		enum al_err (*connected_cb)(void *arg,
		struct pfm_net_tcp *tcp, enum al_err err));

/**
 * Set the receive callback for receiving packets that arrive on the PCB.
 *
 * \param tcp is the TCP PCB.
 * \param recvd_cb is the callback function which will be called when packets
 * arrive on the TCP connection.
 *
 * The receive callback's first argument is the value set by
 * pfm_net_tcp_set_arg().
 * The second argument is the TCP PCB.
 * The third argument is a byte array buffer containing the data, or NULL when
 * an error or close event occurs.
 * The fourth argument is the length of the data received.
 */
void pfm_net_tcp_set_recv_cb(struct pfm_net_tcp *tcp,
		enum al_err (*recvd_cb)(void *arg, struct pfm_net_tcp *tcp,
			void *buf, size_t len));

/**
 * Set the callback argument for a TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \param arg is the opaque argument to be passed to receive, accept,
 * connected, and sent callbacks.
 */
void pfm_net_tcp_set_arg(struct pfm_net_tcp *tcp, void *arg);

/**
 * Indicate that bytes have been received on a TCP connection.
 *
 * This call indicates that len bytes have been received and can be
 * acknowledged by the TCP stack.  If the stack doesn't provide this
 * functionality and automatically acknowledges any data received,
 * this call can be a no-op.
 *
 * \param tcp is the TCP PCB.
 * \param len is the number of bytes to be acknowledged.
 */
void pfm_net_tcp_recved(struct pfm_net_tcp *tcp, size_t len);

/**
 * Close a TCP PCB and the associated connection.
 *
 * Close the TCP connection, freeing the PCB eventually.
 * Once this call is made, no further callbacks should be made.
 *
 * \param tcp is the TCP PCB.
 * \returns zero on success, error code (TBD) on error.
 */
enum al_err pfm_net_tcp_close(struct pfm_net_tcp *tcp);

/**
 * Abort a TCP PCB and the associated connection.
 *
 * Send a reset or FIN to terminate the connection.
 * Once this call is made, no further callbacks should be made.
 *
 * \param tcp is the TCP PCB.
 * \returns zero on success, error code (TBD) on error.
 */
void pfm_net_tcp_abort(struct pfm_net_tcp *tcp);

/**
 * Abandon the TCP connection.  Send a reset if the reset argument is non-zero.
 *
 * TBD: distinguishe between close, abort and abandon.
 *
 * \param tcp is the TCP PCB.
 * \param reset non-zero indicates a TCP reset should be sent
 * on the connnection.
 */
void pfm_net_tcp_abandon(struct pfm_net_tcp *tcp, int reset);

/**
 * Queue the supplied buffer as data to be sent on the PCB.
 *
 * Actual transmission may or may not be delayed as more data accumulates.
 * This returns AE_BUF if the send buffer (TCP send window) is full,
 * or if a packet could not be allocated, without queuing anything.
 * In this case the caller should retry after a sent callback occurs.
 *
 * \param tcp is the TCP PCB.
 * \param buf is a pointer to the data to be sent.
 * \param len is the length of data to be sent, in bytes.
 * \returns zero on success, error code (TBD) on error.
 */
enum al_err pfm_net_tcp_write(struct pfm_net_tcp *tcp,
		const void *buf, size_t len);

/**
 * Send any queued data on the PCB.
 *
 * \param tcp is the TCP PCB.
 * \returns zero on success, error code (TBD) on error.
 *
 * This sends any data that has been buffered on the PCB.  This should be
 * called after all the pfm_net_tcp_write() calls are complete for a request,
 * for example, or when a response is complete.
 * This call should not block waiting for output to be sent.
 */
enum al_err pfm_net_tcp_output(struct pfm_net_tcp *tcp);

/**
 * Set a callback to be called when data is sent on the PCB.
 *
 * \param tcp is the TCP PCB.
 * \param sent_cb is a function to be called when data is sent and acknowledged.
 *
 * The sent callback's first argument is the value set by pfm_net_tcp_set_arg().
 * The second argument is the TCP PCB.
 * The third argument is the number of bytes sent.
 *
 * The send callback is used to indicate to the upper level that
 * data has been sent and acknowledged, and that it may be possible to
 * send more data.  This is useful after a call to pfm_net_tcp_write() or
 * pfm_net_tcp_output() has returned AE_BUF.
 */
void pfm_net_tcp_set_sent_cb(struct pfm_net_tcp *tcp,
		void (*sent_cb)(void *arg, struct pfm_net_tcp *tcp,
		    size_t len));

/**
 * Set the error callback.
 *
 * \param tcp is the TCP PCB.
 * \param err_cb is the callback function which will be called when
 * send & receive error occurs.
 *
 * The receive callback's first argument is the value set by
 * pfm_net_tcp_set_arg().
 * The second argument is the TCP PCB.
 * The third argument is the error code.
 */
void pfm_net_tcp_set_err_cb(struct pfm_net_tcp *tcp,
		void (*err_cb)(void *arg, struct pfm_net_tcp *tcp,
				enum al_err err));

/**
 * Get the local IP address for a TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \returns a pointer to the local IP address of the PCB.
 * It will not return NULL.
 */
const struct al_net_addr *pfm_net_tcp_local_ip(struct pfm_net_tcp *tcp);

/**
 * Get the remote IP address for a TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \returns a pointer to the remote IP address of the PCB.
 * It will not return NULL.
 *
 * If the PCB is not connected, it will return a pointer
 * to a zero address (IN_ADDR_ANY), or the previously-connected address.
 */
const struct al_net_addr *pfm_net_tcp_remote_ip(struct pfm_net_tcp *tcp);

/**
 * Get the connection status of a TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \returns non-zero if the connection is established (connected).
 */
int pfm_net_tcp_is_established(struct pfm_net_tcp *tcp);

/**
 * Set the TCP keep-alive policy for a TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \param idle is the maximum time in ms the PCB be is allowed to be idled
 * before a  keep-alive is sent.
 * \param intvl is the default time in ms between keep-alive probes,
 * \param count is the default number of unanswered keep-alive probes after
 * which the connection will be closed.
 *
 * Discussion: not all platforms will support this.
 * The internal policies of the
 * SDK can override these settings.
 * We currently use this on incoming HTTP server connections so that the PCBs
 * don't hang around for too long after the mobile app drops from the AP.
 * This could be done internally by the stack on pfm_net_tcp_accept().
 */
void pfm_net_tcp_keep_alive_set(struct pfm_net_tcp *tcp,
		unsigned int idle, unsigned int intvl,
		unsigned int count);

/**
 * Set TCP options.
 *
 * Set optional flags for a TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \param flags is the bitwise OR of the options desired.
 *
 * Options available are:
 * AL_NET_TCP_OPT_KEEPALIVE	to enable sending of keep-alives.
 * AL_NET_TCP_OPT_REUSEADDR	to allow reuse of local address and port,
 * AL_NET_TCP_OPT_LINGER to allow socket to stay open at close if data present.
 *
 * Discussion: we currently only use this to enable keep-alive, which could
 * be combined with pfm_net_tcp_keep_alive_set().  TBD.
 */
void pfm_net_tcp_options_set(struct pfm_net_tcp *tcp, unsigned int flags);

/**
 * Return the number of unacknowledged bytes queued on the TCP PCB.
 *
 * \param tcp is the TCP PCB.
 * \returns the number of bytes not acknowledged by the peer on the send buffer.
 */
unsigned int pfm_net_tcp_sndqueuelen(struct pfm_net_tcp *tcp);

/**
 * Determine whether the TCP PCB uses IPv6.
 *
 * \param tcp is the TCP PCB.
 * \returns non-zero if the TCP PCB is using IPv6.
 */
int pfm_net_tcp_v6(struct pfm_net_tcp *tcp);

/**
 * Set a TCP PCB to use IPv6 or IPv4.
 *
 * \param tcp is the TCP PCB.
 * \param v6 is non-zero to use IPv6, zero to use IPv4.
 *
 * Sets the TCP PCB to use IPv6 when accepting connections or making them.
 */
void pfm_net_tcp_set_v6(struct pfm_net_tcp *tcp, int v6);

/**
 * Set low level IO (send/recv) procedure (callback) for net-tcp object.
 *
 * \param tcp is the TCP PCB.
 * \param low_level_recv is a function pointer for receiving from socket.
 * \param low_level_send is a function pointer for sending to socket.
 *
 * Arguments of low_level_recv():
 * The first argument is a pointer to the upper layer.
 * The second argument is a pointer to struct pfm_net_tcp.
 * The third argument is a buffer for received data.
 * The fourth argument is the size of the buffer.
 * The return value of low_level_recv() is the size of data received. Zero
 * or negative means error.
 *
 * Arguments of low_level_send():
 * The first argument is a pointer to the upper layer.
 * The second argument is a pointer to struct pfm_net_tcp.
 * The third argument is a pointer to data to be sent.
 * The fourth argument is the size of data to be sent.
 * The return value of low_level_sent() is the size of data actually sent.
 * Nero or negative means error.
 */
void pfm_net_tcp_set_low_level_io_proc(struct pfm_net_tcp *tcp,
	int (*low_level_recv)(void *, struct pfm_net_tcp *, char *, int),
	int (*low_level_send)(void *, struct pfm_net_tcp *, const char *, int));

/**
 * Get socket handle in struct pfm_net_tcp. The upper layer will use the
 * handle to communicate to the remote.
 *
 * \param tcp is the TCP PCB.
 *
 * \returns the socket handle.
 */
int pfm_net_tcp_get_sock_fd(struct pfm_net_tcp *tcp);

void pfm_net_sync_read(int socket, void *arg);
void pfm_net_sync_write(int socket, void *arg);
void pfm_net_raw_exception(int socket, void *arg);

#endif /* __AYLA_PFM_COMMON_NET_TCP_H__ */
