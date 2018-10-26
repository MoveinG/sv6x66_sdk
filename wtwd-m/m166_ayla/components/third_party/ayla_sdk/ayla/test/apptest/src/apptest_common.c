/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <al/al_ada_thread.h>
#include <al/al_cli.h>
#include <al/al_net_if.h>
#include <al/al_persist.h>
#include <ada/ada.h>
#include <ada/ada_conf.h>
#include "apptest_demo.h"
#include "apptest_conf.h"
#include "apptest.h"

static const char demo_show_help[] = "[version | wifi]";
static const char demo_setup_mode_help[] = "enable | disable | show [<key>]";
static const char test_prop_cli_help[] =
	"test-prop <get|set|show> <prop_name> [prop_val]";
static const char test_show_cli_help[] =
	"test-show <sys|res|prop>";

char conf_sys_model[CONF_MODEL_MAX] = "AY008PWB1";

u8 conf_connected;

int adap_wifi_get_ssid(void *buf, size_t len)
{
	return 0;
}

int adap_net_get_signal(int *signal)
{
	return -1;
}

void adap_conf_reg_changed(void)
{
}

void adap_conf_reset(int factory)
{
}

static int load_conf_string(const char *name, char *buf, size_t size)
{
	ssize_t len;
	len = al_persist_data_read(AL_PERSIST_FACTORY, name, buf, size);
	if (len < 0) {
		buf[0] = '\0';
		return -1;
	} else {
		buf[len] = '\0';
		return 0;
	}
}

static int demo_conf_init(struct ada_conf *conf)
{
	char buf_port[10];
	char *endptr;

	/* Load configuration parameters from persistent storage. How to
	 * persist the parameters depends on the product design. On PWB Linux,
	 * a python tool, conf-gen.py, generates a Json file in the folder
	 * ~/.pwb.conf that is the persistent storage. Here we use al_persist
	 * APIs to load parameters and pass them to the ADA.
	*/
	load_conf_string("service_region", conf->region, sizeof(conf->region));
	if (load_conf_string("device_dsn", conf->dsn, sizeof(conf->dsn))) {
		return -1;
	}
	conf->pubkey_len = al_persist_data_read(AL_PERSIST_FACTORY,
	    "device_key", conf->pubkey, sizeof(conf->pubkey));
	if (conf->pubkey_len <= 0) {
		return -1;
	}
	if (load_conf_string("oem_id", conf->oem, sizeof(conf->oem))) {
		return -1;
	}
	if (al_persist_data_read(AL_PERSIST_FACTORY, "oem_key",
		conf->oemkey, sizeof(conf->oemkey)) != sizeof(conf->oemkey)) {
		return -1;
	}
	if (load_conf_string("oem_model", conf->model, sizeof(conf->model))) {
		return -1;
	}
	load_conf_string("mfg_serial", conf->mfg_serial,
	    sizeof(conf->mfg_serial));
	load_conf_string("mfg_model", conf->mfg_model,
	    sizeof(conf->mfg_model));
	load_conf_string("conf_server", conf->conf_server,
	    sizeof(conf->conf_server));
	load_conf_string("conf_port", buf_port, sizeof(buf_port));
	if (buf_port[0]) {
		conf->ads_port = strtoul(buf_port, &endptr, 10);
	} else {
		conf->ads_port = 443;
	}

	conf->poll_interval = 30;
	conf->get_all = 1;

	al_net_if_get_mac_addr(al_get_net_if(AL_NET_IF_DEF), conf->mac_addr);

	return 0;
}

static void demo_show_cli(int argc, char **argv)
{
	if (argc != 2) {
		goto usage;
	}

	if (!strcmp(argv[1], "version")) {
		printcli("%s", ada_version);
		return;
	}
#ifdef ADA_BUILD_WIFI_SUPPORT
	if (!strcmp(argv[1], "wifi")) {
		adw_wifi_show();
		return;
	}
#endif

usage:
	printcli("usage: show [version|wifi]");
}

/* ada_client_up and ada_client_down should be called by connection manager,
 * such as the ADW. It is called by CLI commands.
 */
static void up_cli(int argc, char **argv)
{
	ada_api_call(ADA_CLIENT_UP);
}

static void down_cli(int argc, char **argv)
{
	ada_api_call(ADA_CLIENT_DOWN);
}

static void register_cli_commands(void)
{
	al_cli_register("conf", conf_cli_help, conf_cli);
	al_cli_register("log", ada_log_cli_help, ada_log_cli);
	al_cli_register("reset", ada_reset_cli_help, ada_reset_cli);
	al_cli_register("show", demo_show_help, demo_show_cli);

	al_cli_register("test-show", test_show_cli_help,
		test_show_cmd);
	al_cli_register("test-prop", test_prop_cli_help,
		test_prop_cmd);

	al_cli_register("up", NULL, up_cli);
	al_cli_register("down", NULL, down_cli);
}

void ada_demo_main(void)
{
	struct ada_conf demo_conf;
	int rc;

	conf_init();
	log_init();
	log_thread_id_set(TASK_LABEL_CLIENT);

	register_cli_commands();

	memset(&demo_conf, 0, sizeof(demo_conf));
	if (demo_conf_init(&demo_conf)) {
		log_put(LOG_ERR "Can not load manufactory configuration!");
		ASSERT_NOTREACHED();
	}
	// rc = ada_api_call(ADA_CLIENT_INIT, &demo_conf);
	if (rc) {
		log_put(LOG_ERR "ADA init failed");
		return;
	}

	/* Schedule init */
	apptest_sched_conf_load();
	ada_api_call(ADA_SCHED_ENABLE);

	/* Demo init */
	apptest_demo_init();
	apptest_demo_ota_init();

	al_ada_main_loop();

#ifdef ADA_BUILD_FINAL
	apptest_demo_final();
	ada_final();
	log_final();
	conf_final();
#endif
}
