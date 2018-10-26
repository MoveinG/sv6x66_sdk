/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_net_stream.h>
#include <platform/pfm_net_tls.h>

struct al_net_stream {
	struct pfm_net_tcp *tcp;
};

struct al_net_stream *al_net_stream_new(enum al_net_stream_type type)
{
	return NULL;
}

enum al_err al_net_stream_close(struct al_net_stream *ps)
{
	return AL_ERR_ERR;
}

void al_net_stream_set_arg(struct al_net_stream *ps, void *arg)
{
}

enum al_err al_net_stream_connect(struct al_net_stream *ps,
	 const char *hostname, struct al_net_addr *peer_addr, u16 port,
	 enum al_err (*conn_cb)(void *arg, struct al_net_stream *,
	 enum al_err err))
{
	return AL_ERR_ERR;
}

int al_net_stream_is_established(struct al_net_stream *ps)
{
	return 0;
}

void al_net_stream_continue_recv(struct al_net_stream *ps)
{
}

void al_net_stream_set_recv_cb(struct al_net_stream *ps,
	enum al_err (*recv_cb)(void *, struct al_net_stream *,
	void *, size_t))
{
}

void al_net_stream_recved(struct al_net_stream *ps, size_t len)
{
}

enum al_err al_net_stream_write(struct al_net_stream *ps,
	const void *buf, size_t len)
{
	return AL_ERR_ERR;
}

enum al_err al_net_stream_output(struct al_net_stream *ps)
{
	return AL_ERR_ERR;
}

void al_net_stream_set_sent_cb(struct al_net_stream *ps,
	void(*sent_cb)(void *, struct al_net_stream *, size_t))
{
}

void al_net_stream_set_err_cb(struct al_net_stream *ps,
	void (*err_cb)(void *arg, enum al_err err))
{
}

struct pfm_net_tcp *al_net_stream_get_tcp_obj(struct al_net_stream *ps)
{
	return NULL;
}
