/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 *
 */
#ifndef __AYLA_AL_COMMON_HTTPD_H__
#define __AYLA_AL_COMMON_HTTPD_H__

/**
 * @file
 * User interface of HTTPD (HTTP server).
 *
 * HTTPD works in single thread mode. Actually it works in the ADA thread.
 * After it is started, server app should call al_httpd_reg_urls_handler()
 * to register handler. When a remote client connects to the HTTPD server,
 * the connection is accepted and an al_httpd_conn structure is created.
 * When a HTTP request is received, the request header is parsed by HTTPD,
 * and the registered handler is called. The request payload is handled by
 * the handler. The argument of the handler is pointer to A structure of
 * al_httpd_conn.
 */
#include <stdlib.h>
#include <ayla/utypes.h>

/**
 * HTTP method or request type.
 */
enum al_http_method {
	AL_REQ_BAD = 0,
	AL_REQ_GET,
	AL_REQ_GET_HEAD,
	AL_REQ_POST,
	AL_REQ_PUT,
	AL_REQ_DELETE,
};

/**
 * The structure is the context used for client connection.
 */
struct al_httpd_conn;

/**
 * Initialize HTTPD module.
 */
void al_httpd_init(void);

/**
 * Finalize HTTPD module.
 */
void al_httpd_final(void);

/**
 * Start an HTTPD server. The HTTPD works in ADA thread.
 * \param port: service port
 * \return status code: 0 is success
 */
int al_httpd_start(u16 port);

/**
 * Stop httpd server
 */
void al_httpd_stop(void);

/**
 * Register a callback for a specified url and method.
 * \param url: HHTP url
 * \param method: Index of HTTP method.
 * \param cb: a callback function to handle the url request.
 * \return status code: 0 is success.
 */
int al_httpd_reg_url_cb(const char *url, enum al_http_method method,
		int (*cb)(struct al_httpd_conn *conn));

/**
 * Get method ("GET", "POST") in the current request.
 * \param conn: pointer to connection context.
 * \return an pointer to the method string in the request header.
 */
const char *al_httpd_get_method(struct al_httpd_conn *conn);

/**
 * Get url in the current request.
 * \param conn: pointer to connection context.
 * \return an pointer to the url string in the request header. NULL is
 * not found.
 * Note: For an url like "http://my-query?a=123&b=hello", the returned
 * url is "http://my-query". To get argument a and b, please call:
 *	val1 = al_httpd_get_arg(struct al_httpd_conn *conn, "a");
 *	val2 = al_httpd_get_arg(struct al_httpd_conn *conn, "b");
 */
const char *al_httpd_get_url(struct al_httpd_conn *conn);

/**
 * Get argument value in the current request's url.
 * \param conn: pointer to connection context.
 * \param name: argument name.
 * \return an pointer to the value string of the argument. NULL is not found.
 */
const char *al_httpd_get_url_arg(struct al_httpd_conn *conn, const char *name);

/**
 * Get HTTP protocol version in the current request.
 * \param conn: pointer to connection context.
 * \return an pointer to the http version string in request header.
 */
const char *al_httpd_get_version(struct al_httpd_conn *conn);

/**
 * Get an field in the request header.
 * \param conn: pointer to connection context
 * \param name: name of the item. It is case sensitive.
 * \return a pointer to the filed content in the request header.
 *
 * Note: if the name is "Date", and the received header line is
 * "Date:xxxxxxxx\r\n", then it returns a pointer to "xxxxxxxx".
 */
const char *al_httpd_get_req_header(struct al_httpd_conn *conn,
	const char *name);

/**
 * Read the payload in the current request.
 * \param conn: pointer to connection context
 * \param buf: buffer for data read
 * \param buf_size: size of the buffer
 * \return the number of bytes read. 0 if reached the end of the data.
 * Negative is error.
 */
int al_httpd_read(struct al_httpd_conn *conn, char *buf, size_t buf_size);

/**
 * Send HTTP response header to the client.
 * \param conn: pointer to connection context
 * \param status: HTTP status code.
 * \param headers: string of response headers. The headers are separated
 *  by '\\r\\n'. The last header is ended in '\\r\\n' or '\\0'. It can be
 * NULL if no additional headers need to be specified.
 * \return status code: 0 is success.
 */
int al_httpd_response(struct al_httpd_conn *conn, int status,
	const char *headers);

/**
 * Write HTTP response payload.
 * \param conn: pointer to connection context
 * \param data: data to be written
 * \param size: size of the data
 * \return status code: 0 is success.
 */
int al_httpd_write(struct al_httpd_conn *conn, const char *data, size_t size);

/**
 * Close httpd client connection.
 * \param conn: pointer to connection context
 */
void al_httpd_close_conn(struct al_httpd_conn *conn);

/*\@}*/

#endif /* __AYLA_AL_COMMON_HTTPD_H__ */

