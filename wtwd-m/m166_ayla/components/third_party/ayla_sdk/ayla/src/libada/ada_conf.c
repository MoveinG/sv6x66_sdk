/*
 * Copyright 2013-2015 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <ayla/utypes.h>
#include <ayla/clock.h>
#include <al/al_ada_thread.h>
#include <al/al_os_reboot.h>
#include <al/al_persist.h>
#include <ada/ada.h>

struct ada_conf ada_conf;
u8 conf_was_reset = 1;
u8 ada_conf_reset_factory;
struct al_ada_callback ada_conf_reset_cb;
#ifdef ADA_BUILD_LAN_SUPPORT
struct ada_lan_conf ada_lan_conf;
#endif

void ada_conf_reset(int factory)
{
	if (factory) {
		al_persist_data_erase(AL_PERSIST_STARTUP);
	}
	adap_conf_reset(factory);
	al_os_reboot();
}

static void ada_conf_reset_callback(void *arg)
{
	ada_conf_reset(ada_conf_reset_factory);
}

static int ada_conf_commit(int from_ui)
{
	timezone_info.valid = 1;
	return 0;
}

/*
 * Initialize config subsystem.
 */
void ada_conf_init(void)
{
	al_ada_callback_init(&ada_conf_reset_cb, ada_conf_reset_callback, NULL);
	ada_conf_register(CONF_PNAME_SYS_RESET, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_BOOL, &conf_was_reset, sizeof(conf_was_reset),
	    NULL, 2, CT_sys, CT_reset);
	ada_conf_register(NULL, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_UTF8, &ada_conf.dsn, sizeof(ada_conf.dsn),
	    NULL, 2, CT_id, CT_dev_id);

	ada_conf_register(CONF_PNAME_TIME_ZONE, NULL, 0,
	    ATLV_INT, &timezone_info.mins, sizeof(timezone_info.mins),
	    ada_conf_commit, 2, CT_sys, CT_timezone);
	ada_conf_register(CONF_PNAME_TIME_ZONE, NULL, 0,
	    ATLV_BOOL, &timezone_info.valid, sizeof(timezone_info.valid),
	    ada_conf_commit, 2, CT_sys, CT_timezone_valid);
	ada_conf_register(CONF_PNAME_TIME_ZONE, NULL, 0,
	    ATLV_BOOL, &daylight_info.valid, sizeof(daylight_info.valid),
	    ada_conf_commit, 2, CT_sys, CT_dst_valid);
	ada_conf_register(CONF_PNAME_TIME_ZONE, NULL, 0,
	    ATLV_BOOL, &daylight_info.active, sizeof(daylight_info.active),
	    ada_conf_commit, 2, CT_sys, CT_dst_active);
	ada_conf_register(CONF_PNAME_TIME_ZONE, NULL, 0,
	    ATLV_UINT, &daylight_info.change, sizeof(daylight_info.change),
	    ada_conf_commit, 2, CT_sys, CT_dst_change);
}
