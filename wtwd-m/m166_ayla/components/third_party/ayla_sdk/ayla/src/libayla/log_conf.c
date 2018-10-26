/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <ayla/conf.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <al/al_log.h>

int log_conf_init(void)
{
	int i;

	for (i = 0; i < LOG_MOD_CT; i++) {
		if (al_log_get_mod_name(i)) {
			conf_register(CONF_PNAME_LOG_MOD, NULL, 0, ATLV_UINT,
			    &log_mods[i].mask, sizeof(log_mods[i].mask),
			    NULL, 4, CT_log, CT_mod, i, CT_mask);
		}
	}
	conf_register(CONF_PNAME_LOG_CLIENT, NULL, 0, ATLV_UINT,
	    &log_client_conf_enabled, sizeof(log_client_conf_enabled),
	    NULL, 3, CT_log, CT_client, CT_enable);
	return 0;
}
