/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#include <lwip/sockets.h>

#include <ayla/log.h>
#include <ayla/assert.h>
#include <al/al_os_mem.h>
#include <al/al_os_lock.h>
#include <al/al_ada_thread.h>
// #include <platform/pfm_pwb_linux.h>
#include <platform/pfm_net_tcp.h>

#define PFM_NET_BUFSIZE	(4096)

#define tcp_log(level, tcp, fmt, arg...) log_put(level \
    "tcp_id %d " fmt, tcp->id, ##arg)

enum pfm_net_tcp_state{
	PFM_NET_TCP_DISCONNECTED,
	PFM_NET_TCP_CONNECTED,
	PFM_NET_TCP_LISTENING,
};

struct pfm_net_frame{
	char data[PFM_NET_BUFSIZE];
	size_t len;

	struct pfm_net_frame *p_next;
};
/**
 * structure representing a TCP connection.
 *
 * This implement of pfm_net_tcp is not thread-safety, all
 * functions of pfm_net_tcp should be called in one thread.
 */
struct pfm_net_tcp{
	int sockfd;
	unsigned int id;
	enum pfm_net_tcp_state state;

	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;

	enum al_err (*accepted_cb)(void *arg, struct pfm_net_tcp *new_tcp,
	    enum al_err err);
	enum al_err (*connected_cb)(void *arg,
	    struct pfm_net_tcp *tcp, enum al_err err);
	enum al_err (*recvd_cb)(void *arg, struct pfm_net_tcp *tcp,
	    void *buf, size_t len);
	void (*sent_cb)(void *arg, struct pfm_net_tcp *tcp, size_t len);
	void (*err_cb)(void *arg, struct pfm_net_tcp *tcp, enum al_err err);

	void *arg;

	struct pfm_net_frame *queue_head;
	struct pfm_net_frame *queue_tail;
	/* because the queue is wrote by tcp thread(may be main thread),
	 * and sent by main thread, so we need to protect the queue */
	struct al_lock *queue_lock;

	int (*low_level_recv)(void *arg, struct pfm_net_tcp *tcp,
		char *buf, int size);
	int (*low_level_send)(void *arg, struct pfm_net_tcp *tcp,
		const char *buf, int size);

	struct al_ada_callback connect_callback;
};

static unsigned int pfm_net_tcp_id;

void pfm_net_sync_read(int socket, void *arg)
{
	struct pfm_net_tcp *p_tcp;
	enum al_err rc_err;
	char ibuf[PFM_NET_BUFSIZE];
	int rc;

	p_tcp = (struct pfm_net_tcp *)arg;
	ASSERT(NULL != p_tcp);
	if (p_tcp->low_level_recv) {
		rc = p_tcp->low_level_recv(p_tcp->arg, p_tcp,
			ibuf, sizeof(ibuf));
	} else {
		rc = recv(p_tcp->sockfd, ibuf, sizeof(ibuf), 0);
	}

	tcp_log(LOG_DEBUG, p_tcp, "recv(%d, %zd) return %d",
		p_tcp->sockfd, sizeof(ibuf), rc);

	if (rc <= 0) {
		p_tcp->state = PFM_NET_TCP_DISCONNECTED;
		al_ada_remove_socket(p_tcp->sockfd);
		rc_err = (rc) ? (AL_ERR_ERR) : (AL_ERR_CLSD);

		if (NULL != p_tcp->err_cb) {
			p_tcp->err_cb(p_tcp->arg, p_tcp, rc_err);
		}
		return;
	}
	if (NULL != p_tcp->recvd_cb) {
		p_tcp->recvd_cb(p_tcp->arg, p_tcp, ibuf, rc);
	}
}

void pfm_net_sync_write(int socket, void *arg)
{
	struct pfm_net_tcp *p_tcp;
	struct pfm_net_frame *p_frame;
	enum al_err rc_err;
	int rc;

	p_tcp = (struct pfm_net_tcp *)arg;
	ASSERT(NULL != p_tcp);

	al_os_lock_lock(p_tcp->queue_lock);
	p_frame = p_tcp->queue_head;
	if (NULL == p_frame) {
		p_tcp->queue_tail = NULL;

		/* if no data need to be sent, disable writeable event */
		al_ada_enable_write(socket, 0);
		goto on_exit;
	}
	ASSERT(0 != p_frame->len);

	if (p_tcp->low_level_send) {
		rc = p_tcp->low_level_send(p_tcp->arg, p_tcp,
			p_frame->data, p_frame->len);
	} else {
		rc = send(p_tcp->sockfd, p_frame->data, p_frame->len, 0);
	}
	tcp_log(LOG_DEBUG, p_tcp, "send(%d, %zd) return %d",
			p_tcp->sockfd, p_frame->len, rc);
	if (rc < 0) {
		rc_err = AL_ERR_CLSD;
		p_tcp->state = PFM_NET_TCP_DISCONNECTED;
		al_ada_remove_socket(p_tcp->sockfd);
		goto on_failed;
	} else if (0 == rc) {
		goto on_exit;
	}
	/* If rc is less than a whole frame's length, move the rest
	 * data to the front of the frame, else remove the frame
	 * from the sending queue.
	 * */
	ASSERT(rc <= p_frame->len);
	if (rc < (int)p_frame->len) {
		memmove(p_frame->data, (p_frame->data + rc),
		    (p_frame->len - rc));
		p_frame->len -= rc;
	} else {
		p_tcp->queue_head = p_frame->p_next;
		al_os_mem_free(p_frame);

		if (p_frame == p_tcp->queue_tail) {
			p_tcp->queue_tail = NULL;

			/* if all data have sent, disable writeable event */
			al_ada_enable_write(socket, 0);
		}
	}
	al_os_lock_unlock(p_tcp->queue_lock);
	if (NULL != p_tcp->sent_cb) {
		p_tcp->sent_cb(p_tcp->arg, p_tcp, rc);
	}
	return;
on_failed:
	al_os_lock_unlock(p_tcp->queue_lock);
	if (NULL != p_tcp->err_cb) {
		p_tcp->err_cb(p_tcp->arg, p_tcp, rc_err);
	}
	return;
on_exit:
	al_os_lock_unlock(p_tcp->queue_lock);
}

void pfm_net_raw_exception(int socket, void *arg)
{
	struct pfm_net_tcp *p_tcp;

	p_tcp = (struct pfm_net_tcp *)arg;
	ASSERT(NULL != p_tcp);

	p_tcp->state = PFM_NET_TCP_DISCONNECTED;
	al_ada_remove_socket(p_tcp->sockfd);
	if (NULL != p_tcp->err_cb) {
		p_tcp->err_cb(p_tcp->arg, p_tcp, AL_ERR_ERR);
	}
}

static void pfm_net_sync_accept(int socket, void *arg)
{
	struct pfm_net_tcp *p_tcp, *p_new;
	enum al_err ret_err;
	socklen_t addr_len;
	int sockfd, rc;

	p_tcp = (struct pfm_net_tcp *)arg;
	ASSERT(NULL != p_tcp);
	ASSERT(NULL != p_tcp->accepted_cb);

	p_new = (struct pfm_net_tcp *)al_os_mem_calloc(
	    sizeof(struct pfm_net_tcp));
	if (NULL == p_tcp) {
		ret_err = AL_ERR_BUF;
	} else {
		addr_len = sizeof(p_new->remote_addr);
		sockfd = accept(p_tcp->sockfd, \
		    (struct sockaddr *)&p_new->remote_addr, \
		    &addr_len);
		if (sockfd < 0) {
			ret_err = AL_ERR_ERR;
			al_os_mem_free(p_new);
			p_new = NULL;
		} else {
			ret_err = AL_ERR_OK;
			p_new->state = PFM_NET_TCP_CONNECTED;

			p_new->sockfd = sockfd;
			p_new->id = pfm_net_tcp_id++;

			addr_len = sizeof(p_new->local_addr);
			rc = getsockname(p_new->sockfd, \
			    (struct sockaddr *)&p_new->local_addr, &addr_len);
			ASSERT(0 == rc);
			p_new->queue_lock = al_os_lock_create();
			ASSERT(p_new->queue_lock);
			rc = al_ada_add_socket(p_new->sockfd,
			    pfm_net_sync_read,
			    pfm_net_sync_write,
			    pfm_net_raw_exception,
			    (void *)p_new);
			ASSERT(0 == rc);

			tcp_log(LOG_DEBUG, p_tcp,
			    "accept (%s:%d) new socket id(%d)...",
			    inet_ntoa(p_new->remote_addr.sin_addr), \
			    ntohs(p_new->remote_addr.sin_port), p_new->id);
		}
	}

	p_tcp->accepted_cb(p_tcp->arg, p_new, ret_err);
}


void pfm_net_tcp_sync_connect(void *argv)
{
	struct pfm_net_tcp *tcp;
	struct sockaddr_in *p_addr;
	socklen_t addr_len;
	enum al_err rc_err;
	int rc;

	ASSERT(NULL != argv);

	tcp = (struct pfm_net_tcp *)argv;

	p_addr = &tcp->remote_addr;
	addr_len = sizeof(tcp->remote_addr);
		
	tcp_log(LOG_DEBUG, tcp, "connecting (%s:%d)...",
	    inet_ntoa(tcp->remote_addr.sin_addr), \
	    ntohs(tcp->remote_addr.sin_port));
	/* TODO: Need to be asynchronous */
	rc = connect(tcp->sockfd,
	    (struct sockaddr *)p_addr, addr_len);
	tcp_log(LOG_DEBUG, tcp, "connect return %d.", rc);
	if (0 != rc) {
		rc_err = AL_ERR_ERR;
	} else {
		rc_err = AL_ERR_OK;
		tcp->state = PFM_NET_TCP_CONNECTED;

		addr_len = sizeof(tcp->local_addr);
		rc = getsockname(tcp->sockfd, \
		    (struct sockaddr *)&tcp->local_addr, &addr_len);
		ASSERT(0 == rc);

		rc = al_ada_add_socket(tcp->sockfd,
		    pfm_net_sync_read,
		    pfm_net_sync_write,
		    pfm_net_raw_exception,
		    (void *)tcp);
		ASSERT(0 == rc);
	}

	if (NULL != tcp->connected_cb) {
		tcp->connected_cb(tcp->arg, tcp, rc_err);
	}
}
struct pfm_net_tcp *pfm_net_tcp_new(void)
{
	int sockfd;
	struct pfm_net_tcp *p_tcp;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return NULL;
	}

	p_tcp = (struct pfm_net_tcp *)al_os_mem_calloc(
	    sizeof(struct pfm_net_tcp));
	if (NULL == p_tcp) {
		close(sockfd);
	} else {
		p_tcp->sockfd = sockfd;
		p_tcp->queue_lock = al_os_lock_create();
		ASSERT(p_tcp->queue_lock);
		p_tcp->id = pfm_net_tcp_id++;
	}
	al_ada_callback_init(&p_tcp->connect_callback,
	    pfm_net_tcp_sync_connect, (void *)p_tcp);
	return p_tcp;
}

enum al_err pfm_net_tcp_bind(struct pfm_net_tcp *tcp,
	struct al_net_addr *addr, unsigned short port)
{
	struct sockaddr_in *p_addr;
	int rc;

	ASSERT(NULL != tcp);
	ASSERT(NULL != addr);
	ASSERT(0 != port);

	p_addr = &tcp->local_addr;
	memset(p_addr, 0x0, sizeof(*p_addr));
	p_addr->sin_family = AF_INET;
	p_addr->sin_addr.s_addr = addr->ip;
	p_addr->sin_port = htons(port);
	rc = bind(tcp->sockfd, (struct sockaddr *)p_addr, sizeof(*p_addr));
	tcp_log(LOG_DEBUG, tcp, "bind(%s, %d) return %d",
	    inet_ntoa(addr->ip), port, rc);
	if (0 != rc) {
		return AL_ERR_ERR;
	} else {
		return AL_ERR_OK;
	}
}

struct pfm_net_tcp *pfm_net_tcp_listen(struct pfm_net_tcp *tcp, int backlog)
{
	int rc;

	ASSERT(NULL != tcp);

	rc = listen(tcp->sockfd, backlog);
	if (0 != rc) {
		tcp_log(LOG_DEBUG, tcp, "listen(%d, %d) return %d",
		    tcp->sockfd, backlog, rc);
		return NULL;
	}
	rc = al_ada_add_socket(tcp->sockfd,
	    pfm_net_sync_accept,
	    NULL,
	    pfm_net_raw_exception,
	    (void *)tcp);
	ASSERT(0 == rc);
	tcp_log(LOG_DEBUG, tcp, "listen(%s, %d) return %d",
	    inet_ntoa(tcp->local_addr.sin_addr),
	    tcp->local_addr.sin_port, rc);
	tcp->state = PFM_NET_TCP_LISTENING;

	return (0 == rc) ? tcp : NULL;
}

void pfm_net_tcp_accept(struct pfm_net_tcp *tcp,
    enum al_err (*accepted_cb)(void *arg,
    struct pfm_net_tcp *new_tcp, enum al_err err))
{
	ASSERT(NULL != tcp);
	tcp->accepted_cb = accepted_cb;
}

enum al_err pfm_net_tcp_connect(struct pfm_net_tcp *tcp,
    struct al_net_addr *addr, unsigned short port,
    enum al_err (*connected_cb)(void *arg,
    struct pfm_net_tcp *tcp, enum al_err err))
{
	struct sockaddr_in *p_addr;
	ASSERT(NULL != tcp);
	ASSERT(NULL != addr);
	ASSERT(0 != port);

	tcp->connected_cb = connected_cb;

	p_addr = &tcp->remote_addr;
	memset(p_addr, 0x0, sizeof(*p_addr));
	p_addr->sin_family = AF_INET;
	p_addr->sin_addr.s_addr = addr->ip;
	p_addr->sin_port = htons(port);

	al_ada_call(&tcp->connect_callback);
	return AL_ERR_OK;
}

void pfm_net_tcp_set_recv_cb(struct pfm_net_tcp *tcp,
    enum al_err (*recvd_cb)(void *arg, struct pfm_net_tcp *tcp,
    void *buf, size_t len))
{
	ASSERT(NULL != tcp);

	tcp->recvd_cb = recvd_cb;
}

void pfm_net_tcp_set_arg(struct pfm_net_tcp *tcp, void *arg)
{
	ASSERT(NULL != tcp);

	tcp->arg = arg;
}

void pfm_net_tcp_recved(struct pfm_net_tcp *tcp, size_t len)
{
}

enum al_err pfm_net_tcp_close(struct pfm_net_tcp *tcp)
{
	struct pfm_net_frame *frame, *p_next;
	ASSERT(NULL != tcp);

	tcp_log(LOG_DEBUG, tcp, "close(%d) .", tcp->sockfd);
	if (PFM_NET_TCP_DISCONNECTED != tcp->state) {
		al_ada_remove_socket(tcp->sockfd);
	}
	close(tcp->sockfd);

	al_os_lock_lock(tcp->queue_lock);
	frame = tcp->queue_head;
	while (frame) {
		p_next = frame->p_next;
		al_os_mem_free(frame);
		frame = p_next;
	}
	al_os_lock_unlock(tcp->queue_lock);
	al_os_lock_destroy(tcp->queue_lock);
	al_os_mem_free(tcp);
	return AL_ERR_OK;
}

void pfm_net_tcp_abort(struct pfm_net_tcp *tcp)
{
}

void pfm_net_tcp_abandon(struct pfm_net_tcp *tcp, int reset)
{
}

enum al_err pfm_net_tcp_write(struct pfm_net_tcp *tcp,
		const void *buf, size_t len)
{
	struct pfm_net_frame *p_frame;
	enum al_err ret_err;
	char *p_dst;
	size_t cp_size;

	ASSERT(NULL != tcp);
	ASSERT(NULL != buf);
	ASSERT(0 != len);

	al_os_lock_lock(tcp->queue_lock);
	while (len) {
		p_frame = tcp->queue_tail;
		if (NULL != p_frame && \
			    p_frame->len < sizeof(p_frame->data)) {
			cp_size = sizeof(p_frame->data) - p_frame->len;
			if (cp_size > len) {
				cp_size = len;
			}
			p_dst = p_frame->data + p_frame->len;
		} else {
			p_frame = (struct pfm_net_frame *)al_os_mem_calloc(
			    sizeof(struct pfm_net_frame));
			if (NULL == p_frame) {
				ret_err = AL_ERR_BUF;
				goto on_exit;
			}
			cp_size = (sizeof(p_frame->data) > len) ? \
			    (len) : (sizeof(p_frame->data));
			p_dst = p_frame->data;
			if (NULL == tcp->queue_head) {
				tcp->queue_head = p_frame;
				tcp->queue_tail = p_frame;
			} else {
				tcp->queue_tail->p_next = p_frame;
				tcp->queue_tail = p_frame;
			}
		}
		memcpy(p_dst, buf, cp_size);
		p_frame->len += cp_size;
		buf += cp_size;
		len -= cp_size;
	}

	ret_err = AL_ERR_OK;
on_exit:
	al_os_lock_unlock(tcp->queue_lock);
	return ret_err;
}

enum al_err pfm_net_tcp_output(struct pfm_net_tcp *tcp)
{
	ASSERT(NULL != tcp);

	al_ada_enable_write(tcp->sockfd, 1);
	return AL_ERR_OK;
}

void pfm_net_tcp_set_sent_cb(struct pfm_net_tcp *tcp,
		void (*sent_cb)(void *arg, struct pfm_net_tcp *tcp, size_t len))
{
	ASSERT(NULL != tcp);

	tcp->sent_cb = sent_cb;
}

void pfm_net_tcp_set_err_cb(struct pfm_net_tcp *tcp,
		void (*err_cb)(void *arg, struct pfm_net_tcp *tcp,
				enum al_err err))
{
	ASSERT(NULL != tcp);

	tcp->err_cb = err_cb;
}

const struct al_net_addr *pfm_net_tcp_local_ip(struct pfm_net_tcp *tcp)
{
	ASSERT(NULL != tcp);

	return (struct al_net_addr *)&tcp->local_addr.sin_addr;
}

const struct al_net_addr *pfm_net_tcp_remote_ip(struct pfm_net_tcp *tcp)
{
	ASSERT(NULL != tcp);

	return (struct al_net_addr *)&tcp->remote_addr.sin_addr;
}

int pfm_net_tcp_is_established(struct pfm_net_tcp *tcp)
{
	ASSERT(NULL != tcp);

	return (PFM_NET_TCP_CONNECTED == tcp->state);
}

void pfm_net_tcp_keep_alive_set(struct pfm_net_tcp *tcp,
	unsigned int idle, unsigned int intvl,
	unsigned int count)
{
}

void pfm_net_tcp_options_set(struct pfm_net_tcp *tcp, unsigned int flags)
{
}

unsigned int pfm_net_tcp_sndqueuelen(struct pfm_net_tcp *tcp)
{
	return -1;
}

int pfm_net_tcp_v6(struct pfm_net_tcp *tcp)
{
	return 0;
}

void pfm_net_tcp_set_v6(struct pfm_net_tcp *tcp, int v6)
{
}

void pfm_net_tcp_set_low_level_io_proc(struct pfm_net_tcp *tcp,
	int (*low_level_recv)(void *, struct pfm_net_tcp *, char *, int),
	int (*low_level_send)(void *, struct pfm_net_tcp *, const char *, int))
{
	ASSERT(tcp);
	tcp->low_level_recv = low_level_recv;
	tcp->low_level_send = low_level_send;
}

int pfm_net_tcp_get_sock_fd(struct pfm_net_tcp *tcp)
{
	ASSERT(tcp);
	return tcp->sockfd;
}
