/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_net_dns.h>
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/err.h"
#include <al/al_ada_thread.h>

enum al_err al_dns_req_ipv4_start(struct al_net_dns_req *req)
{
	int try_times = 3;
	struct hostent *hp = NULL;
	while(hp == NULL && try_times > 0){
		hp = lwip_gethostbyname(req->hostname);
		try_times++;
		OS_MsDelay(100);
	}	
	if (hp) {
		struct ip4_addr *ip4_addr = (struct ip4_addr *)hp->h_addr;		
		req->addr.ip = ip4_addr->addr;
		printf("[%s][%d] host：%s  addr:%s\n",__FUNCTION__,__LINE__,req->hostname,inet_ntoa(req->addr.ip));		
		if(req->callback){
			req->callback(req);
		}
		return AL_ERR_OK;
	}else{
		return AL_ERR_ERR;
	}
	
}

void al_dns_req_cancel(struct al_net_dns_req *req)
{
}

void al_net_dns_delete_host(const char *hostname)
{
}

void al_net_dns_servers_rotate(void)
{
}
