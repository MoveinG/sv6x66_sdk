/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_net_udp.h>

struct al_net_udp *al_net_udp_new(void)
{
	return NULL;
}

enum al_err al_net_udp_bind(struct al_net_udp *udp,
		struct al_net_addr *addr, u16 port)
{
	return AL_ERR_ERR;
}

enum al_err al_net_udp_connect(struct al_net_udp *udp,
		struct al_net_addr *addr, u16 port)
{
	return AL_ERR_ERR;
}

void al_net_udp_set_recv_cb(struct al_net_udp *udp,
	void (*recv_cb)(void *arg, struct al_net_udp *,
	void *buf, size_t len,
	struct al_net_addr *from_ip, unsigned short from_port,
	struct al_net_if *net_if), void *recv_arg)
{
}

void *al_net_udp_buf_alloc(size_t len)
{
	return NULL;
}

void al_net_udp_buf_free(void *mbuf)
{
}

enum al_err al_net_udp_send(struct al_net_udp *udp, void *buf,
	size_t len)
{
	return AL_ERR_ERR;
}

enum al_err al_net_udp_sendto_if(struct al_net_udp *udp, void *buf,
	size_t len, struct al_net_addr *to, unsigned short port,
	struct al_net_if *nif)
{
	return AL_ERR_ERR;
}

void al_net_udp_free(struct al_net_udp *udp)
{
}

enum al_err al_net_igmp_joingroup(struct al_net_udp *udp,
		struct al_net_addr *if_addr,
		struct al_net_addr *group)
{
	return AL_ERR_ERR;
}

enum al_err al_net_igmp_leavegroup(struct al_net_udp *udp,
		struct al_net_addr *if_addr,
		struct al_net_addr *group)
{
	return AL_ERR_ERR;
}
