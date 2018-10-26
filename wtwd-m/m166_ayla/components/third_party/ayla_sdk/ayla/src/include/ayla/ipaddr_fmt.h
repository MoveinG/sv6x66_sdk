/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef _NET_IPADDR_FMT_H_
#define _NET_IPADDR_FMT_H_

u32 str_to_ipv4(const char *addr);

char *ipv4_to_str(u32 addr, char *buf, int buflen);

#endif
