/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ayla/assert.h>
#include <al/al_ada_thread.h>
#include <al/al_os_mem.h>
#include <al/al_net_udp.h>
// #include <platform/pfm_pwb_linux.h>

#include <sys/types.h>
#include "lwip/sockets.h"
// #include <net/if.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <ifaddrs.h>
// #include <linux/if_link.h>

#define AL_UDP_BUFSIZE	256
#define MOD_LOG_UDP	MOD_LOG_NOTIFY

struct al_udp_mbuf {
	void *buf; /* allocated by al_net_udp_buf_alloc() */
	size_t len;

	/* Fields for al_net_udp_sendto_if() */
	struct al_net_addr addr_to;
	unsigned short port_to;
	struct al_net_if *net_if;

	struct al_udp_mbuf *next;
};

struct al_net_udp {
	int id;
	int fd; /* socket */
	struct al_net_addr remote_addr;
	u16 remote_port;
	struct al_net_if *net_if;
	void *recv_arg;
	void (*recv_cb)(void *arg, struct al_net_udp *udp,
		void *buf, size_t len,
		struct al_net_addr *from_ip,
		unsigned short from_port,
		struct al_net_if *net_if);
	struct al_udp_mbuf *mbuf;
};

#define udp_log(level, udp, fmt, arg...) log_put_mod((u8)MOD_LOG_UDP,\
	level "udp (id=%d) " fmt, (udp) ? udp->id : 0, ##arg)

static void al_net_udp_add_mbuf_to_list(struct al_net_udp *udp,
	struct al_udp_mbuf *mbuf)
{
	struct al_udp_mbuf *node;
	ASSERT(udp);
	if (udp->mbuf == NULL) {
		udp->mbuf = mbuf;
	} else {
		for (node = udp->mbuf; node; node = node->next) {
			if (node->next == NULL) {
				node->next = mbuf;
				break;
			}
		}
	}
}

static struct al_udp_mbuf *al_net_udp_get_mbuf_from_list(struct al_net_udp *udp)
{
	static struct al_udp_mbuf *mbuf;

	mbuf = udp->mbuf;
	if (!mbuf)
		return NULL;
	udp->mbuf = mbuf->next;
	return mbuf;
}

static void al_net_udp_free_mbuf_list(struct al_net_udp *udp)
{
	struct al_udp_mbuf *node;
	struct al_udp_mbuf *next;
	int cnt = 0;
	ASSERT(udp);
	for (cnt = 0, node = udp->mbuf; node; cnt++) {
		node = node->next;
	}
	for (node = udp->mbuf; node; ) {
		if (node->buf) {
			al_net_udp_buf_free(node->buf);
		}
		next = node->next;
		al_os_mem_free(node);
		node = next;
	}
	udp->mbuf = NULL;
}

static void al_net_udp_raw_recv(int socket, void *arg)
{
	struct al_net_udp *udp;
	char tmp_buf[AL_UDP_BUFSIZE];
	struct sockaddr_in sa;
	socklen_t addr_len;
	void *buf;
	int rc;

	ASSERT(arg);
	udp = (struct al_net_udp *)arg;
	ASSERT(udp->fd > 0);
	ASSERT(udp->fd == socket);
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	/* sa.sin_port = htons(udp->remote_port); */
	/* sa.sin_addr = udp->remote_addr.sin_addr; */
	addr_len = sizeof(sa);
	memset(tmp_buf, 0, sizeof(tmp_buf));
	rc = recvfrom(udp->fd, tmp_buf, sizeof(tmp_buf), 0,
		(struct sockaddr *)&sa, &addr_len);
	if (rc <= 0) {
		udp_log(LOG_ERR, udp, "%s( from = 0x%08lX, port = %d ): "
			"recvfrom() error", __func__,
			al_net_addr_get_ipv4(&udp->remote_addr),
			udp->remote_port);
		/* pfm_pwb_linux_remove_socket(udp->fd); */
		return;
	}
	if (udp->recv_cb == NULL) {
		udp_log(LOG_DEBUG, udp, "%s( from = 0x%08lX, port = %d ): "
			"udp->recv_cb = null", __func__,
			al_net_addr_get_ipv4(&udp->remote_addr),
			udp->remote_port);
		return;
	}
	buf = al_net_udp_buf_alloc(rc);
	ASSERT(buf);

	memcpy(buf, tmp_buf, rc);
	udp->recv_cb(udp->recv_arg, udp, buf, rc,
		&udp->remote_addr,
		udp->remote_port, udp->net_if);
	udp_log(LOG_DEBUG, udp, "%s( from = 0x%08lX, port = %d, len = %d )",
		__func__, al_net_addr_get_ipv4(&udp->remote_addr),
		udp->remote_port, rc);
}

static void al_net_udp_raw_send(int socket, void *arg)
{
	struct al_net_udp *udp;
	struct al_udp_mbuf *mbuf;
	struct al_net_addr *paddr;
	unsigned short port;
	struct sockaddr_in sa;
	int n;

	ASSERT(arg);
	udp = (struct al_net_udp *)arg;
	ASSERT(udp->fd > 0);
	ASSERT(udp->fd == socket);

	/* get a buffered packet */
	mbuf = al_net_udp_get_mbuf_from_list(udp);
	if (!mbuf) {
		/* all buffered packets have been sent */
		return;
	}
	if (mbuf->net_if) {
		port = mbuf->port_to;
		paddr = &(mbuf->addr_to);
	} else {
		port = udp->remote_port;
		paddr = &(udp->remote_addr);
	}
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = paddr->ip;
	udp_log(LOG_DEBUG, udp, "%s( to = 0x%08lX, port = %d, len = %d )",
		__func__, al_net_addr_get_ipv4(paddr), port, mbuf->len);

	n = sendto(socket, mbuf->buf, mbuf->len, 0,
		(const struct sockaddr *) &sa, sizeof(sa));
	if (n != mbuf->len) {
		udp_log(LOG_ERR, udp, "%s(): sendto() error", __func__);
	}
	al_net_udp_buf_free(mbuf->buf);
	al_os_mem_free(mbuf);
	if (udp->mbuf == NULL) {
		al_ada_enable_write(udp->fd, 0);
	}
}

static void al_net_udp_raw_exception(int socket, void *arg)
{
	struct al_net_udp *udp;
	ASSERT(arg);
	udp = (struct al_net_udp *)arg;
	udp_log(LOG_ERR, udp, "%s( sock=%d, udp = %p )",
		__func__, socket, udp);
}

struct al_net_udp *al_net_udp_new(void)
{
	static u16 gv_udp_obj_count;
	struct al_net_udp *udp;

	udp = al_os_mem_calloc(sizeof(*udp));
	if (udp == NULL)
		goto clean;
	udp->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp->fd <= 0)
		goto clean;
	udp->id = ++gv_udp_obj_count;
	udp_log(LOG_DEBUG, udp, "%s() = %p", __func__, udp);
	return udp;
clean:
	if (udp) {
		if (udp->fd > 0)
			close(udp->fd);
		al_os_mem_free(udp);
	}
	return NULL;
}

enum al_err al_net_udp_bind(struct al_net_udp *udp,
		struct al_net_addr *addr, u16 port)
{
	int rc;
	struct sockaddr_in sa;

	ASSERT(udp);
	ASSERT(udp->fd > 0);
	ASSERT(addr);
	ASSERT(port);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = addr->ip;

	/* Associates a local address & port with a socket */
	rc = bind(udp->fd, (struct sockaddr *)&sa, sizeof(sa));
	rc = (rc) ? AL_ERR_ERR : AL_ERR_OK;
	udp_log(LOG_DEBUG, udp, "%s( addr = 0x%08lX, port = %d ) = %d",
		__func__, al_net_addr_get_ipv4(addr), port, rc);
	return rc;
}

enum al_err al_net_udp_connect(struct al_net_udp *udp,
		struct al_net_addr *addr, u16 port)
{
	int rc;

	ASSERT(udp);
	ASSERT(addr);
	ASSERT(udp->fd > 0);
	ASSERT(port);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = addr->ip;
	rc = connect(udp->fd, (struct sockaddr *)&sa, sizeof(sa));
	if (rc == AL_ERR_OK) {
		udp->remote_addr = *addr;
		udp->remote_port = port;
		rc = al_ada_add_socket(udp->fd,
			al_net_udp_raw_recv,
			al_net_udp_raw_send,
			al_net_udp_raw_exception,
			(void *)udp);
	} else {
		udp_log(LOG_ERR, udp, "connect() = %s", strerror(errno));
	}
	rc = (rc) ? AL_ERR_ERR : AL_ERR_OK;
	udp_log(LOG_DEBUG, udp, "%s( fd = %d, addr = 0x%08lX, port = %d ) = %d",
		__func__, udp->fd, al_net_addr_get_ipv4(addr), port, rc);
	return rc;
}

void al_net_udp_set_recv_cb(struct al_net_udp *udp,
		void (*recv_cb)(void *arg, struct al_net_udp *udp,
		void *buf, size_t len, struct al_net_addr *from_ip,
		unsigned short from_port, struct al_net_if *net_if),
		void *recv_arg)
{
	ASSERT(udp);
	ASSERT(recv_cb);
	udp->recv_cb = recv_cb;
	udp->recv_arg = recv_arg;
}

void *al_net_udp_buf_alloc(size_t len)
{
	void *p;
	ASSERT(0 < len && len <= AL_UDP_BUFSIZE);
	p = al_os_mem_alloc(len);
	return p;
}

void al_net_udp_buf_free(void *buf)
{
	al_os_mem_free(buf);
}

enum al_err al_net_udp_send(struct al_net_udp *udp, void *buf, size_t len)
{
	struct al_udp_mbuf *mbuf;

	ASSERT(udp);
	ASSERT(buf != NULL);
	ASSERT(len != 0);

	mbuf = al_os_mem_calloc(sizeof(*mbuf));
	if (!mbuf) {
		return AL_ERR_ALLOC;
	}
	mbuf->buf = buf;
	mbuf->len = len;
	al_net_udp_add_mbuf_to_list(udp, mbuf);
	al_ada_enable_write(udp->fd, 1);
	udp_log(LOG_DEBUG, udp, "%s( len = %d )", __func__, len);
	return AL_ERR_OK;
}

enum al_err al_net_udp_sendto_if(struct al_net_udp *udp, void *buf, size_t len,
		struct al_net_addr *to, unsigned short port,
		struct al_net_if *nif)
{
	struct al_udp_mbuf *mbuf;

	ASSERT(udp);
	ASSERT(buf != NULL);
	ASSERT(len != 0);
	ASSERT(to != NULL);
	ASSERT(port != 0);
	ASSERT(nif != NULL);

	mbuf = al_os_mem_calloc(sizeof(*mbuf));
	if (!mbuf) {
		return AL_ERR_ALLOC;
	}
	mbuf->buf = buf;
	mbuf->len = len;
	mbuf->addr_to = *to;
	mbuf->port_to = port;
	mbuf->net_if = nif;
	al_net_udp_add_mbuf_to_list(udp, mbuf);
	al_ada_enable_write(udp->fd, 1);
	udp_log(LOG_DEBUG, udp, "%s( to = 0x%08lX, port = %d, len = %d )",
		__func__, al_net_addr_get_ipv4(to), port, len);
	return AL_ERR_OK;
}

void al_net_udp_free(struct al_net_udp *udp)
{
	if (udp) {
		udp_log(LOG_DEBUG, udp, "%s( %p )", __func__, udp);
		if (udp->fd > 0) {
			al_ada_remove_socket(udp->fd);
			close(udp->fd);
		}
		al_net_udp_free_mbuf_list(udp);
		al_os_mem_free(udp);
	}
}

enum al_err al_net_igmp_joingroup(struct al_net_udp *udp,
		struct al_net_addr *if_addr,
		struct al_net_addr *group)
{
	struct ip_mreq mreq;
	int rc;

	ASSERT(udp);
	memset(&mreq, 0, sizeof(mreq));
	if (group)
		mreq.imr_multiaddr.s_addr = group->ip;
	if (if_addr)
		mreq.imr_interface.s_addr = if_addr->ip;
	rc = setsockopt(udp->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char *)&mreq, sizeof(mreq));
	rc = (rc < 0) ? AL_ERR_ERR : AL_ERR_OK;
	udp_log(LOG_DEBUG, udp, "%s( ifa = 0x%08lX, grp = 0x%08lX ) = %d",
		__func__, if_addr ? al_net_addr_get_ipv4(if_addr) : 0,
		group ? al_net_addr_get_ipv4(group) : 0, rc);
	return rc;
}

enum al_err al_net_igmp_leavegroup(struct al_net_udp *udp,
		struct al_net_addr *if_addr,
		struct al_net_addr *group)
{
	struct ip_mreq mreq;
	int rc;

	ASSERT(udp);
	memset(&mreq, 0, sizeof(mreq));
	if (group)
		mreq.imr_multiaddr.s_addr = group->ip;
	if (if_addr)
		mreq.imr_interface.s_addr = if_addr->ip;
	rc = setsockopt(udp->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		&mreq, sizeof(mreq));
	rc = (rc < 0) ? AL_ERR_ERR : AL_ERR_OK;
	udp_log(LOG_DEBUG, udp, "%s( ifa = 0x%08lX, grp = 0x%08lX ) = %d",
		__func__, if_addr ? al_net_addr_get_ipv4(if_addr) : 0,
		group ? al_net_addr_get_ipv4(group) : 0, rc);
	return rc;
}

