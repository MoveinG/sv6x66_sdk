/*
 * Copyright 2011-2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <ayla/utypes.h>
#include <ada/ada.h>

/* Reminder
 *
 * CT_client, CT_server, CT_region: all : dropped
 * CT_client, CT_server, CT_hostname: all : dropped
 * CT_client, CT_server, CT_port: all : dropped
 * CT_client, CT_gif: all : dropped
 * CT_client, CT_reg, CT_start: set : dropped <reset reg_token>
 * CT_client, CT_reg: CT_interval : set : dropped <start reg_window>
 * CT_client, CT_user. CT_reg: set : dropped
 *
 * client_conf_reset_at_commit is dropped, need think in the future.
 * client_conf_commit() is dropped
 */

static u8 conf_ssl_enable = 1;
static u8 conf_server_enable;

#ifdef ADA_BUILD_LAN_SUPPORT
static int conf_lan_persist_enable(void)
{
	return ada_lan_conf.enable;
}
#endif

static int conf_client_poll_interval_commit(int from_ui)
{
	if (ada_conf.poll_interval < 5) {
		ada_conf.poll_interval = 5;
	}
	return 0;
}

static int conf_client_server_default_commit(int from_ui)
{
	return CONF_COMMIT_RET_REBOOT;
}

void client_conf_init(void)
{
#ifdef ADA_BUILD_LAN_SUPPORT
	/* Lan mode configuration */
	ada_conf_register(CONF_PNAME_LAN_MODE, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_BOOL, &ada_lan_conf.enable, sizeof(ada_lan_conf.enable),
	    NULL, 2, CT_lan, CT_enable);

	ada_conf_register(CONF_PNAME_LAN_MODE, conf_lan_persist_enable,
	    CONF_REG_FLAG_READONLY, ATLV_UTF8,
	    &ada_lan_conf.lanip_key, sizeof(ada_lan_conf.lanip_key),
	    NULL, 2, CT_lan, CT_private_key);

	ada_conf_register(CONF_PNAME_LAN_MODE, conf_lan_persist_enable,
	    CONF_REG_FLAG_READONLY, ATLV_UINT,
	    &ada_lan_conf.lanip_key_id, sizeof(ada_lan_conf.lanip_key_id),
	    NULL, 2, CT_lan, CT_key);

	ada_conf_register(CONF_PNAME_LAN_MODE, conf_lan_persist_enable,
	    CONF_REG_FLAG_READONLY, ATLV_UINT,
	    &ada_lan_conf.keep_alive, sizeof(ada_lan_conf.keep_alive),
	    NULL, 2, CT_lan, CT_poll_interval);

	ada_conf_register(CONF_PNAME_LAN_MODE, conf_lan_persist_enable,
	    CONF_REG_FLAG_READONLY, ATLV_BOOL,
	    &ada_lan_conf.auto_echo, sizeof(ada_lan_conf.auto_echo),
	    NULL, 2, CT_lan, CT_auto);
#endif

	ada_conf_register(CONF_PNAME_CLIENT, NULL, 0, ATLV_BOOL,
	    &ada_conf.conf_serv_override, sizeof(ada_conf.conf_serv_override),
	    conf_client_server_default_commit,
	    3, CT_client, CT_server, CT_default);

	ada_conf_register(CONF_PNAME_CLIENT, NULL, 0,
	    ATLV_BOOL, &conf_server_enable, sizeof(conf_server_enable),
	    NULL, 3, CT_client, CT_server, CT_source);

	ada_conf_register(CONF_PNAME_CLIENT, NULL, 0,
	    ATLV_BOOL, &ada_conf.enable, sizeof(ada_conf.enable),
	    NULL, 2, CT_client, CT_enable);

	ada_conf_register(CONF_PNAME_CLIENT, NULL, 0, ATLV_UINT,
	    &ada_conf.poll_interval, sizeof(ada_conf.poll_interval),
	    conf_client_poll_interval_commit, 2, CT_client, CT_poll_interval);

	ada_conf_register(CONF_PNAME_CLIENT, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_BOOL, &ada_conf.reg_user, sizeof(ada_conf.reg_user),
	    NULL, 3, CT_client, CT_reg, CT_ready);

	ada_conf_register(NULL, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_UTF8, &ada_conf.reg_token, sizeof(ada_conf.reg_token),
	    NULL, 2, CT_client, CT_reg);

	ada_conf_register(NULL, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_BOOL, &conf_ssl_enable, sizeof(conf_ssl_enable),
	    NULL, 3, CT_client, CT_ssl, CT_enable);
}

/*
 * CLI command to reset the module, optionally to the factory configuration.
 */
const char ada_reset_cli_help[] = "[factory]";
void ada_reset_cli(int argc, char **argv)
{
	if (argc == 1) {
		ada_conf_reset(0);
		return;		/* not reached */
	}
	if (argc == 2) {
		if (!strcmp(argv[1], "factory")) {
			ada_conf_reset(1);
			return;		/* not reached */
		}
	}
	printcli("usage: reset [factory]\n");
}
