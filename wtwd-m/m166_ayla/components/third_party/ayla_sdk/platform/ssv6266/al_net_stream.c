/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ayla/assert.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>

#include <al/al_net_stream.h>
#include <al/al_os_mem.h>
#include <al/al_os_lock.h>
#include <platform/pfm_net_addr.h>
#include <platform/pfm_net_tls.h>
#include <platform/pfm_net_tcp.h>

#undef NET_STREAM_USE_LIST
#define MOD_LOG_STREAM  MOD_LOG_SSL

struct al_net_stream {
	u16 id;
	u16 refcnt;

	enum al_net_stream_type type;
	struct pfm_net_tcp *tcp;
	struct pfm_net_tls *tls;

	const char *hostname;
	struct al_net_addr remote_addr;
	u16 remote_port;

	void *arg;

	enum al_err (*connected_cb)(void *arg, struct al_net_stream *ps,
		enum al_err err);
	enum al_err (*recvd_cb)(void *arg, struct al_net_stream *ps,
		void *buf, size_t len);
	void (*sent_cb)(void *arg, struct al_net_stream *ps, size_t len);
	void (*err_cb)(void *arg, enum al_err err);

	int connect_req:1;
	int tcp_connected:1;
#ifdef NET_STREAM_USE_LIST
	struct al_net_stream *next;
#endif
	struct al_lock *lock;
};

#ifdef NET_STREAM_USE_LIST

/*static struct al_lock *gv_lock;*/
#define list_lock(lock)
#define list_unlock(lock)

static struct al_net_stream *gp_stream_list;

static void stream_node_remove(struct al_net_stream *ps)
{
	/* Only called by al_net_stream_free. So lock is not required. */
	struct al_net_stream *p;
	if (!ps) {
		return;
	}
	list_lock(gv_lock);
	if (ps == gp_stream_list) {
		gp_stream_list = ps->next;
	} else if (gp_stream_list) {
		for (p = gp_stream_list; p->next; p = p->next) {
			if (ps == p->next) {
				p->next = ps->next;
				break;
			}
		}
	}
	ps->next = NULL;
	list_unlock(gv_lock);
	ASSERT(gp_stream_list != ps);
}

static void stream_node_add(struct al_net_stream *ps)
{
	static u16 gv_id_count;

	list_lock(gv_lock);
	ps->id = ++gv_id_count;
	ps->next = gp_stream_list;
	gp_stream_list = ps;
	list_unlock(gv_lock);
}

#else

#define stream_node_remove(ps)
#define stream_node_add(ps)

#endif /* NET_STREAM_USE_LIST */

/*-------------------------------------------------------------------*/

static const char *stream_type_name(struct al_net_stream *ps)
{
	switch (ps->type) {
	case AL_NET_STREAM_TCP: return "TCP";
	case AL_NET_STREAM_TLS: return "TLS";
	default:
		break;
	}
	return "XXX";
}

static void stream_log(const char *level, struct al_net_stream *ps,
		const char *fmt, ...)
{
	va_list args;
	char buf[LOG_LINE];
	enum log_mod_id mod = MOD_LOG_STREAM;

	if (ps) {
		snprintf(buf, sizeof(buf), "%cstream (id=%d) %s",
			level[0], ps->id, fmt);
		fmt = buf;
	} else {
		snprintf(buf, sizeof(buf), "%cstream (unknown) %s",
			level[0], fmt);
		fmt = buf;
	}
	va_start(args, fmt);
	log_put_va((u8)mod, fmt, args);
	va_end(args);
}

static void al_net_stream_free(struct al_net_stream *ps)
{
	ASSERT(ps->refcnt);
	if (--ps->refcnt == 0) {
		stream_node_remove(ps);
		al_os_mem_free(ps);
	}
}

static void al_net_stream_on_data_sent(void *arg, struct pfm_net_tcp *tcp,
	size_t len)
{
	struct al_net_stream *ps = arg;
	ASSERT(ps);
	ASSERT(ps->tcp == tcp);
	if (ps->sent_cb) {
		/* report to HTTP client */
		stream_log(LOG_DEBUG, ps, "%s( %p, %u)", __func__, ps, len);
		ps->sent_cb(ps->arg, ps, len);
	}
}

static enum al_err al_net_stream_on_data_received(void *arg,
	struct pfm_net_tcp *tcp, void *buf, size_t len)
{
	enum al_err err;
	struct al_net_stream *ps = arg;

	ASSERT(ps);
	ASSERT(ps->tcp == tcp);
	if (ps->recvd_cb) {
		/* report to HTTP client */
		stream_log(LOG_DEBUG, ps, "%s( %p, %u )", __func__, ps, len);
		err = ps->recvd_cb(ps->arg, ps, buf, len);
	} else {
		err = AL_ERR_ERR;
	}
	return err;
}

static void al_net_stream_on_error(void *arg, struct pfm_net_tcp *tcp,
	enum al_err err)
{
	struct al_net_stream *ps = arg;
	ASSERT(ps);
	ASSERT(ps->tcp == tcp);
	stream_log(LOG_DEBUG, ps, "%s( %p ) = %d", __func__, ps, err);
	if (ps->err_cb) {
		/* report to HTTP client */
		ps->err_cb(ps->arg, err);
	}
}

static int al_net_stream_low_level_recv(void *arg, struct pfm_net_tcp *tcp,
	char *buf, int size)
{
	int rc;
	struct al_net_stream *ps = arg;

	ASSERT(ps);
	ASSERT(ps->tcp == tcp);
	ASSERT(ps->type == AL_NET_STREAM_TLS);
	ASSERT(ps->tls);

	al_os_lock_lock(ps->lock);
	rc = pfm_net_tls_get_link_status(ps->tls);
	if (rc != AL_ERR_OK) {
		rc = -1;
		goto on_exit;
	}
	rc = pfm_net_tls_read(ps->tls, buf, size);
on_exit:
	al_os_lock_unlock(ps->lock);
	return rc;
}


static int al_net_stream_low_level_send(void *arg, struct pfm_net_tcp *tcp,
	const char *buf, int size)
{
	int rc;
	struct al_net_stream *ps = arg;

	ASSERT(ps);
	ASSERT(ps->tcp == tcp);
	ASSERT(ps->type == AL_NET_STREAM_TLS);
	ASSERT(ps->tls);

	al_os_lock_lock(ps->lock);
	rc = pfm_net_tls_get_link_status(ps->tls);
	if (rc != AL_ERR_OK) {
		rc = -1;
		goto on_exit;
	}
	rc = pfm_net_tls_write(ps->tls, buf, size);
on_exit:
	al_os_lock_unlock(ps->lock);
	return rc;
}

struct al_net_stream *al_net_stream_new(enum al_net_stream_type type)
{
	struct al_net_stream *ps;
	struct al_lock *lock;

	ASSERT(type >= 0 && type < AL_NET_STREAM_MAX);
	lock = al_os_lock_create();
	al_os_lock_lock(lock);
	ps = (struct al_net_stream *)al_os_mem_calloc(sizeof(*ps));
	if (!ps) {
		stream_log(LOG_WARN, ps, "%s(): calloc failed", __func__);
		al_os_lock_unlock(lock);
		al_os_lock_destroy(lock);
		return NULL;
	}
	ps->tcp = pfm_net_tcp_new();
	ps->lock = lock;
	ps->tls = NULL;
	ps->type = type;
	ps->refcnt = 1;
	pfm_net_tcp_set_arg(ps->tcp, ps);
	stream_node_add(ps);
	stream_log(LOG_DEBUG, ps, "%s( %s ) = %p", __func__,
		stream_type_name(ps), ps);
	al_os_lock_unlock(ps->lock);
	return ps;
}

enum al_err al_net_stream_close(struct al_net_stream *ps)
{
	struct al_lock *lock;
	if (NULL != ps) {
		lock = ps->lock;
		al_os_lock_lock(lock);
		pfm_net_tcp_close(ps->tcp);
		if (ps->tls) {
			pfm_net_tls_free(ps->tls);
			ps->tls = NULL;
		}
		ps->connected_cb = NULL;
		ps->recvd_cb = NULL;
		ps->sent_cb = NULL;
		ps->err_cb = NULL;
		ps->lock = NULL;

		stream_log(LOG_DEBUG, ps, "%s( %s, %p ).", __func__,
			stream_type_name(ps), ps);
		al_net_stream_free(ps);
		al_os_lock_unlock(lock);
		al_os_lock_destroy(lock);
	} else {
		stream_log(LOG_WARN, ps, "%s( %p ).", __func__, ps);
	}
	return AL_ERR_OK;
}

void al_net_stream_set_arg(struct al_net_stream *ps, void *arg)
{
	ASSERT(ps);
	ps->arg = arg;
}

static enum al_err al_net_stream_on_connected(void *arg,
	struct pfm_net_tcp *tcp, enum al_err err)
{
	struct al_net_stream *ps = arg;
	int fd;

	ASSERT(ps);
	ASSERT(ps->tcp == tcp);
	ASSERT(ps->connect_req);
	ASSERT(ps->tcp_connected == 0);
	al_os_lock_lock(ps->lock);
	ps->tcp_connected = (err == AL_ERR_OK);
	switch (ps->type) {
	case AL_NET_STREAM_TCP:
		break;
	case AL_NET_STREAM_TLS:
		if (ps->tcp_connected) {
			fd = pfm_net_tcp_get_sock_fd(ps->tcp);
			ASSERT(fd >= 0);
			al_ada_remove_socket(fd);
			ps->tls = pfm_net_tls_handshake(fd, ps->hostname,
				10000);
			if (ps->tls) {
				pfm_net_tcp_set_low_level_io_proc(ps->tcp,
					al_net_stream_low_level_recv,
					al_net_stream_low_level_send);

				al_ada_add_socket(fd,
					pfm_net_sync_read,
					pfm_net_sync_write,
					NULL,
					(void *)tcp);
			}
			
			err = (ps->tls) ? AL_ERR_OK : AL_ERR_TIMEOUT;
		}
		break;
	default:
		ASSERT_NOTREACHED();
		break;
	}
	stream_log(LOG_DEBUG, ps, "%s( %s ) = %d", __func__,
		stream_type_name(ps), err);
	al_os_lock_unlock(ps->lock);
	if (ps->connected_cb) {
		ps->connected_cb(ps->arg, ps, err);
	}
	return AL_ERR_OK;
}

enum al_err al_net_stream_connect(struct al_net_stream *ps,
	const char *hostname, struct al_net_addr *peer_addr, u16 port,
	enum al_err (*conn_cb)(void *arg, struct al_net_stream *,
	enum al_err err))
{
	u32 peer_ipv4;
	enum al_err err;
	ASSERT(ps);
	ASSERT(hostname);
	ASSERT(conn_cb);
	ASSERT(port);
	peer_ipv4 = al_net_addr_get_ipv4(peer_addr);
	ASSERT(0 != peer_ipv4 && (u32)-1 != peer_ipv4);
	al_os_lock_lock(ps->lock);
	if (ps->tcp_connected != 0) {
		err = AL_ERR_BUSY;
		goto on_exit;
	}
	if (ps->connect_req != 0) {
		err = AL_ERR_IN_PROGRESS;
		goto on_exit;
	}
	ps->connect_req = 1;
	ps->hostname = hostname;
	ps->remote_addr.ip = peer_ipv4;
	ps->remote_port = port;
	ps->connected_cb = conn_cb;
	switch (ps->type) {
	case AL_NET_STREAM_TCP:
	case AL_NET_STREAM_TLS:
		ASSERT(ps->tcp);
		err = pfm_net_tcp_connect(ps->tcp, peer_addr, port,
			al_net_stream_on_connected);
		break;
	default:
		ASSERT_NOTREACHED();
		err = AL_ERR_INVAL_TYPE;
		break;
	}
on_exit:
	al_os_lock_unlock(ps->lock);
	if (err) {
		stream_log(LOG_DEBUG, ps, "%s() = %d", __func__, err);
	}
	return err;
}

int al_net_stream_is_established(struct al_net_stream *ps)
{
	int rc;

	ASSERT(ps);
	switch (ps->type) {
	case AL_NET_STREAM_TCP:
		rc = ps->tcp_connected;
		break;
	case AL_NET_STREAM_TLS:
		rc = (ps->tls != NULL);
		break;
	default:
		ASSERT_NOTREACHED();
		rc = 0;
		break;
	}
	return rc;
}

void al_net_stream_continue_recv(struct al_net_stream *ps)
{
	ASSERT(ps);
}

void al_net_stream_set_recv_cb(struct al_net_stream *ps,
	enum al_err (*recv_cb)(void *, struct al_net_stream *,
	void *, size_t))
{
	ASSERT(ps);
	ASSERT(recv_cb);
	al_os_lock_lock(ps->lock);
	ps->recvd_cb = recv_cb;
	switch (ps->type) {
	case AL_NET_STREAM_TCP:
	case AL_NET_STREAM_TLS:
		ASSERT(ps->tcp);
		pfm_net_tcp_set_recv_cb(ps->tcp,
			al_net_stream_on_data_received);
		break;
	default:
		ASSERT_NOTREACHED();
		break;
	}
	al_os_lock_unlock(ps->lock);
}

void al_net_stream_recved(struct al_net_stream *ps, size_t len)
{
	/* Upper layer will use this function to tell net-stream or
	 * net tcp how many data received.
	 */
	ASSERT(ps);
	ASSERT(ps->tcp);
	al_os_lock_lock(ps->lock);
	switch (ps->type) {
	case AL_NET_STREAM_TCP:
	case AL_NET_STREAM_TLS:
		stream_log(LOG_DEBUG, ps, "%s( %p, %u )", __func__, ps, len);
		pfm_net_tcp_recved(ps->tcp, len);
		break;
	default:
		ASSERT_NOTREACHED();
		break;
	}
	al_os_lock_unlock(ps->lock);
}

enum al_err al_net_stream_write(struct al_net_stream *ps,
	const void *buf, size_t len)
{
	enum al_err err;
	ASSERT(ps);
	ASSERT(buf);
	ASSERT(ps->tcp);
	al_os_lock_lock(ps->lock);
	stream_log(LOG_DEBUG, ps, "%s( %p, %u )", __func__, ps, len);
	err = pfm_net_tcp_write(ps->tcp, buf, len);
	al_os_lock_unlock(ps->lock);
	return err;
}

enum al_err al_net_stream_output(struct al_net_stream *ps)
{
	enum al_err err;
	ASSERT(ps);
	ASSERT(ps->tcp);
	stream_log(LOG_DEBUG, ps, "%s( %p )", __func__, ps);
	err = pfm_net_tcp_output(ps->tcp);
	return err;
}

void al_net_stream_set_sent_cb(struct al_net_stream *ps,
	void(*sent_cb)(void *, struct al_net_stream *, size_t))
{
	ASSERT(ps);
	ASSERT(ps->tcp);
	al_os_lock_lock(ps->lock);
	ps->sent_cb = sent_cb;
	pfm_net_tcp_set_sent_cb(ps->tcp, al_net_stream_on_data_sent);
	al_os_lock_unlock(ps->lock);
}

void al_net_stream_set_err_cb(struct al_net_stream *ps,
	void (*err_cb)(void *arg, enum al_err err))
{
	ASSERT(ps);
	ASSERT(err_cb);
	ASSERT(ps->tcp);
	al_os_lock_lock(ps->lock);
	ps->err_cb = err_cb;
	switch (ps->type) {
	case AL_NET_STREAM_TCP:
	case AL_NET_STREAM_TLS:
		pfm_net_tcp_set_err_cb(ps->tcp, al_net_stream_on_error);
		break;
	default:
		ASSERT_NOTREACHED();
		break;
	}
	al_os_lock_unlock(ps->lock);
}

struct pfm_net_tcp *al_net_stream_get_tcp_obj(struct al_net_stream *ps)
{
	ASSERT(ps);
	return ps->tcp;
}
