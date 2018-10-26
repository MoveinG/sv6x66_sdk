/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_NET_STREAM_H__
#define __AYLA_AL_COMMON_NET_STREAM_H__

#include <ayla/utypes.h>
#include <al/al_err.h>
#include <platform/pfm_net_addr.h>

/**
 * @file
 * Network stream interfaces
 */

/*
 * HTTP	    |   HTTPS	    |	COAP
 *------------------------------------
 *		STREAM
 *------------------------------------
 *	    |   NET_TLS	    |	DTLS
 * NET_TCP  |   NET_TCP	    |	COAP
 *	    |		    |	UDP
 *------------------------------------
*/

/**
 * Structure of stream control block.
 */
struct al_net_stream;

/**
 * Type of stream connection.
 */
enum al_net_stream_type {
	AL_NET_STREAM_TCP,   /*!< Use unencrypted HTTP/1.1 */
	AL_NET_STREAM_TLS,   /*!< Use HTTP/1.1 over TLS */
	AL_NET_STREAM_MAX,  /*!< al_net_stream_type limit */
};

/**
 * Open a new stream.
 *
 * \param type is the type of http client. It is also the stream type.
 *
 * \returns a pointer to struct al_net_stream, or NULL on failure.
 */
struct al_net_stream *al_net_stream_new(enum al_net_stream_type type);

/**
 * Close the specified stream.
 *
 * \param stream is the stream to be closed.
 */
enum al_err al_net_stream_close(struct al_net_stream *stream);

/**
 * Set the callback argument for the stream.
 *
 * \param stream is the stream.
 * \param arg is the opaque argument to be passed to receive, accept,
 * connected, and sent callbacks.
 */
void al_net_stream_set_arg(struct al_net_stream *stream, void *arg);

/**
 * Connect to remote server.
 *
 * \param stream is the stream.
 * \param hostname is the remote host name. It is used in certificates
 * checking.
 * \param host_addr is the remote host address.
 * \param port is the remote port.
 * \param connected is a callback which will called on connection ends.
 * The first argument of connected() is the value set by
 * al_net_stream_set_arg().
 * The second argument is a pointer to al_net_stream structure.
 * The third argument is the error number.
 *
 * \returns zero on success, error code on failure.
 */
enum al_err al_net_stream_connect(struct al_net_stream *stream,
	const char *hostname, struct al_net_addr *host_addr, u16 port,
	enum al_err (*connected)(void *arg, struct al_net_stream *,
	enum al_err err));

/**
 * Get the connection status of net stream.
 *
 * \param stream is the stream.
 * \returns non-zero if the connection is established (connected).
 */
int al_net_stream_is_established(struct al_net_stream *stream);

/**
 * continue to receive.
 * If stream call the recv_cb, it may pause receiving, and should
 * call this function before recv_cb returns.
 *
 * \param stream is the stream.
 */
void al_net_stream_continue_recv(struct al_net_stream *stream);
/**
 * Set the receive-callback for receiving data.
 *
 * \param stream is the stream.
 * \param recv_cb is the callback for received data.
 * The first argument of recv_cb() is the value set by
 * al_net_stream_set_arg().
 * The second argument is a pointer to the al_net_stream structure.
 * The third argument is the data received,
 * The fourth argument is the data size.
 */
void al_net_stream_set_recv_cb(struct al_net_stream *stream,
	enum al_err (*recv_cb)(void *arg, struct al_net_stream *stream,
	void *data, size_t size));

/**
 * Indicate that bytes have been received.
 *
 * This call indicates that len bytes have been received and can be
 * acknowledged by the stream. If the stack doesn't provide this
 * functionality and automatically acknowledges any data received,
 * this call can be a no-op.
 *
 * \param stream is the stream.
 * \param len is the number of bytes to be acknowledged.
 */
void al_net_stream_recved(struct al_net_stream *stream, size_t len);

/**
 * Put data to stream buffer. Actually transmission may or may not be delayed.
 *
 * \param stream is the stream.
 * \param data is the data to be sent.
 * \param len is data size.
 * \returns zero on success.
 */
enum al_err al_net_stream_write(struct al_net_stream *stream,
	const void *data, size_t len);

/**
 * Send any queued data buffered in the stream.
 *
 * \param stream is the stream.
 * \returns zero on succss, error code (TBD) on error.
 *
 * This sends any data that has been buffered on the stream. This should be
 * called after all the pfm_net_tcp_write() calls are complete for a request,
 * for example, or when a response is complete.
 * This call should not block waiting for output to be sent.
 */
enum al_err al_net_stream_output(struct al_net_stream *stream);

/**
 * Set a callback to be called when data is sent to remote.
 *
 * \param stream is the stream.
 * \param sent_cb is a callback function to be called when data is sent
 * and acknowledged.
 *
 * The first argument of sent_cb() is the value set by al_net_stream_set_arg().
 * The second argument is stream handle.
 * The third argument is the number of bytes sent.
 */
void al_net_stream_set_sent_cb(struct al_net_stream *stream,
	void(*sent_cb)(void *arg, struct al_net_stream *stream,
	size_t len_sent));

/**
 * Set the error callback.
 *
 * \param stream is the stream.
 * \param err_cb is the callback for reporting error.
 * The first argument of err_cb() is the value set by al_net_stream_set_arg().
 * The second argument is the error number.
 */
void al_net_stream_set_err_cb(struct al_net_stream *stream,
	void (*err_cb)(void *arg, enum al_err err));

/**
 * Get the associated net tcp object. The net tcp object can be used to call
 * net tcp functions to set TCP options. Please do not use the object to send
 * or receive data.
 *
 * \param stream is the stream.
 * \returns a pointer to struct pfm_net_tcp, or NULL on failure.
 */
struct pfm_net_tcp *al_net_stream_get_tcp_obj(struct al_net_stream *stream);

#endif /* __AYLA_AL_COMMON_NET_STREAM_H__ */
