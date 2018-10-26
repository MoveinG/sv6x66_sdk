/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include "ayla/log.h"
#include "ayla/mod_log.h"
#include <al/al_utypes.h>
#include <al/al_log.h>

static const char * const log_mod_names[LOG_MOD_CT] = {
	[MOD_LOG_CLIENT] = "client",
	[MOD_LOG_CONF] = "conf",
#ifdef ADA_BUILD_DNSS_SUPPORT
	[MOD_LOG_DNSS] = "dnss",
#endif
	[MOD_LOG_MOD] = "mod",
#ifdef ADA_BUILD_ANS_SUPPORT
	[MOD_LOG_NOTIFY] = "notify",
#endif
	[MOD_LOG_SERVER] = "server",
	[MOD_LOG_SSL] = "ssl",
#ifdef ADA_BUILD_WIFI_SUPPORT
	[MOD_LOG_WIFI] = "wifi",
#endif
#ifdef ADA_BUILD_LOG_CLIENT_SUPPORT
	[MOD_LOG_LOGGER] = "log-client",
#endif
	[MOD_LOG_SCHED] = "sched",
};

void al_log_print(const char *line)
{
	printf("[ada] %s", line);
}

const char *al_log_get_mod_name(u8 mod_id)
{
	mod_id &= LOG_MOD_MASK;
	if (mod_id >= LOG_MOD_CT) {
		return NULL;
	}
	return log_mod_names[mod_id];
}
