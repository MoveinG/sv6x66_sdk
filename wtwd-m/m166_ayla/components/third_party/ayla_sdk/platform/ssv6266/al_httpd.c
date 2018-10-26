/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_httpd.h>

void al_httpd_init(void)
{
}

void al_httpd_final(void)
{
}

int al_httpd_start(u16 port)
{
	return -1;
}

void al_httpd_stop(void)
{
}

int al_httpd_reg_url_cb(const char *url, enum al_http_method method,
		int (*cb)(struct al_httpd_conn *conn))
{
	return -1;
}

const char *al_httpd_get_method(struct al_httpd_conn *conn)
{
	return NULL;
}

const char *al_httpd_get_url(struct al_httpd_conn *conn)
{
	return NULL;
}

const char *al_httpd_get_url_arg(struct al_httpd_conn *conn, const char *name)
{
	return NULL;
}

const char *al_httpd_get_version(struct al_httpd_conn *conn)
{
	return NULL;
}

const char *al_httpd_get_req_header(struct al_httpd_conn *conn,
	const char *name)
{
	return NULL;
}

int al_httpd_read(struct al_httpd_conn *conn, char *buf, size_t buf_size)
{
	return -1;
}

int al_httpd_response(struct al_httpd_conn *conn, int status,
	const char *headers)
{
	return -1;
}

int al_httpd_write(struct al_httpd_conn *conn, const char *data, size_t size)
{
	return -1;
}

void al_httpd_close_conn(struct al_httpd_conn *conn)
{
}
