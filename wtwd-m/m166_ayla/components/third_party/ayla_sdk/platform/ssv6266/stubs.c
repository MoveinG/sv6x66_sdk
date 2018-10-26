/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <ayla/assert.h>
#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ayla/tlv.h>
#include <ayla/conf.h>
#include <ada/err.h>
#include <ayla/conf.h>
#include <ada/ada_conf.h>
#include <ada/server_req.h>
#include <ada/prop.h>
#include <al/al_ada_thread.h>
#include <al/al_net_if.h>
#include <al/al_net_udp.h>

u8 log_snap_saved;

const char *prop_dp_put_get_loc(void)
{
	ASSERT_NOTREACHED();
	return NULL;
}

enum ada_err prop_dp_put_close(enum prop_cb_status stat, void *arg)
{
	ASSERT_NOTREACHED();
	return AE_ERR;
}

enum ada_err prop_dp_put(enum prop_cb_status stat, void *arg)
{
	ASSERT_NOTREACHED();
	return AE_ERR;
}

enum ada_err prop_dp_req(enum prop_cb_status stat, void *arg)
{
	ASSERT_NOTREACHED();
	return AE_ERR;
}

/* log_client */
void log_client_init(void)
{
}

int log_client_enable(int enable)
{
	return -1;
}

const char *log_client_host(void)
{
	return "";
}

void log_client_set(const char *host, char *uri, const char *protocol)
{
}

void log_client_reset(void)
{
}


/*
 * Allow server thread, waiting in client_lan_sync_wait(), to proceed.
 */
void client_lan_sync_post(void)
{
}

/* conf dummy */
const char *adap_conf_sw_build(void)
{
	return ada_version_build;
}

void client_conf_reg_persist(void)
{
}
