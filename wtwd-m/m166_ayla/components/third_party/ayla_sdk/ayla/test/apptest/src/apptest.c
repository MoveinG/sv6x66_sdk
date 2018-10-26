/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ayla/conf.h>
#include <al/al_cli.h>
#include <al/al_persist.h>
#include <ada/err.h>
#include <ada/ada.h>
#include <ada/ada_conf.h>
#include <ada/sprop.h>
#include <ada/prop_mgr.h>
#include <ada/client_ota.h>

#include "apptest_demo.h"
#include "apptest.h"

/*
 * Test declarations
 */
#define demo_log(_format, ...) test_log(_format, ##__VA_ARGS__)

const char test_show_help[] = "<sys|res|prop>";
const char test_prop_help[] = "<get|set|show> <prop_name> [prop_val]";

/*
 * ADA API Test related definitions
 */
static struct ada_test_api api_test[] = {
	{ "ada_sprop_mgr_register",	610678, 0, API_NOT_TESTED},
	{ "ada_sprop_send",		610679, 0, API_NOT_TESTED},
	{ "ada_sprop_send_by_name",	610680, 0, API_NOT_TESTED},
	{ "ada_sprop_set_bool",		610681, 0, API_NOT_TESTED},
	{ "ada_sprop_get_bool",		610682, 0, API_NOT_TESTED},
	{ "ada_sprop_set_int",		610683, 0, API_NOT_TESTED},
	{ "ada_sprop_get_int",		610684, 0, API_NOT_TESTED},
	{ "ada_sprop_set_uint",		610685, 0, API_NOT_TESTED},
	{ "ada_sprop_get_uint",		610686, 0, API_NOT_TESTED},
	{ "ada_sprop_set_string",	610687, 0, API_NOT_TESTED},
	{ "ada_sprop_get_string",	610688, 0, API_NOT_TESTED},
	{ "ada_sprop_dest_mask",	610689, 0, API_NOT_TESTED},
	{ "ada_prop_mgr_register",	610690, 0, API_NOT_TESTED},
	{ "ada_prop_mgr_ready",		610691, 0, API_NOT_TESTED},
	{ "ada_prop_mgr_request",	610692, 0, API_NOT_TESTED},
	{ "ada_prop_mgr_send",		610693, 0, API_NOT_TESTED},
	{ "send_done",			610694, 0, API_NOT_TESTED},
	{ "connect_status",		610695, 0, API_NOT_TESTED},
	{ "prop_recv",			610696, 0, API_NOT_TESTED},
	{ "client_reg_window_start",	610701, 0, API_NOT_TESTED},
	{ "ada_init",			610698, 0, API_NOT_TESTED},
	{ "ada_client_up",		610699, 0, API_NOT_TESTED},
	{ "ada_client_down",		610700, 0, API_NOT_TESTED},
	{ "ada_sched_init",		610702, 0, API_NOT_TESTED},
	{ "ada_sched_enable",		610703, 0, API_NOT_TESTED},
	{ "ada_sched_set_name",		610704, 0, API_NOT_TESTED},
	{ "ada_sched_get_index",	610705, 0, API_NOT_TESTED},
	{ "ada_sched_set_index",	610706, 0, API_NOT_TESTED},
	{ "ada_ota_register",		610707, 0, API_NOT_TESTED},
	{ "ada_ota_start",		610708, 0, API_NOT_TESTED},
	{ "ada_ota_report",		610709, 0, API_NOT_TESTED},
	{ "log_info",			610710, 0, API_NOT_TESTED},
	{ "log_put",			610711, 0, API_NOT_TESTED},
	{ NULL }
};

/*
 * ADA API test wrapper
 */
ssize_t api_sprop_get_bool(struct ada_sprop *sprop, void *buf, size_t len)
{
	return ada_api_call(ADA_SPROP_GET_BOOL, sprop, buf, len);
}

ssize_t api_sprop_get_int(struct ada_sprop *sprop, void *buf, size_t len)
{
	return ada_api_call(ADA_SPROP_GET_INT, sprop, buf, len);
}

ssize_t api_sprop_get_uint(struct ada_sprop *sprop, void *buf, size_t len)
{
	return ada_api_call(ADA_SPROP_GET_UINT, sprop, buf, len);
}

ssize_t api_sprop_get_string(struct ada_sprop *sprop, void *buf, size_t len)
{
	return ada_api_call(ADA_SPROP_GET_STRING, sprop, buf, len);
}

ssize_t api_sprop_dest_mask()
{
	return ada_api_call(ADA_SPROP_DEST_MASK);
}

enum ada_err ada_api_call(enum apitest_struct id, ...)
{
	struct ada_sprop *sprop;
	struct prop_mgr *pm;
	struct prop *p;
	void *buf;
	char *fmt, *name;
	size_t *psize;
	char **pname;
	unsigned int idx;
	unsigned int len;
	struct ada_ota_ops *pot;
	struct ada_conf *pconf;
	va_list ap;

	va_start(ap, id);

	switch (id) {
	case ADA_SPROP_MGR_REGISTER:
		name = va_arg(ap, char *);
		sprop = va_arg(ap, struct ada_sprop *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_mgr_register(name, sprop, len);
		break;
	case ADA_SPROP_SEND:
		sprop = va_arg(ap, struct ada_sprop *);
		api_test[id].rval = ada_sprop_send(sprop);
		break;
	case ADA_SPROP_SEND_BY_NAME:
		name = va_arg(ap, char *);
		api_test[id].rval = ada_sprop_send_by_name(name);
		break;
	case ADA_SPROP_SET_BOOL:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_set_bool(sprop, buf, len);
		break;
	case ADA_SPROP_GET_BOOL:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_get_bool(sprop, buf, len);
		break;
	case ADA_SPROP_SET_INT:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_set_int(sprop, buf, len);
		break;
	case ADA_SPROP_GET_INT:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_get_int(sprop, buf, len);
		break;
	case ADA_SPROP_SET_UINT:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_set_uint(sprop, buf, len);
		break;
	case ADA_SPROP_GET_UINT:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_get_uint(sprop, buf, len);
		break;
	case ADA_SPROP_SET_STRING:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_set_string(sprop, buf, len);
		break;
	case ADA_SPROP_GET_STRING:
		sprop = va_arg(ap, struct ada_sprop *);
		buf = va_arg(ap, void *);
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sprop_get_string(sprop, buf, len);
		break;
	case ADA_SPROP_DEST_MASK:
		api_test[id].rval = ada_sprop_dest_mask;
		break;
	case ADA_PROP_MGR_REGISTER:
		pm = va_arg(ap, struct prop_mgr *);
		ada_prop_mgr_register(pm);
		api_test[id].rval = 0;
		break;
	case ADA_PROP_MGR_READY:
		pm = va_arg(ap, struct prop_mgr *);
		ada_prop_mgr_ready(pm);
		api_test[id].rval = 0;
		break;
	case ADA_PROP_MGR_REQUEST:
		name = va_arg(ap, char *);
		api_test[id].rval = ada_prop_mgr_request(name);
		break;
	case ADA_PROP_MGR_SEND:
		pm  = va_arg(ap, struct prop_mgr *);
		p   = va_arg(ap, struct prop *);
		idx = va_arg(ap, int);
		buf = va_arg(ap, void *);
		api_test[id].rval = ada_prop_mgr_send(pm, p, idx, buf);
		break;
	case ADA_PROP_SEND_DONE:
		api_test[id].rval = va_arg(ap, int);
		break;
	case ADA_PROP_CONNECT_STATUS:
		api_test[id].rval = va_arg(ap, int);
		api_test[id].rval &= 0xff;
		break;
	case ADA_PROP_RECV:
		api_test[id].rval = va_arg(ap, int);
		break;
	case ADA_CLIENT_REG_WINDOW_START:
		client_reg_window_start();
		api_test[id].rval = 0;
		break;
	case ADA_CLIENT_INIT:
		pconf = va_arg(ap, struct ada_conf *);
		api_test[id].rval = ada_init(pconf);
		break;
	case ADA_CLIENT_UP:
		ada_client_up();
		api_test[id].rval = 0;
		break;
	case ADA_CLIENT_DOWN:
		ada_client_down();
		api_test[id].rval = 0;
		break;
	case ADA_SCHED_INIT:
		len = va_arg(ap, size_t);
		api_test[id].rval = ada_sched_init(len);
		break;
	case ADA_SCHED_ENABLE:
		api_test[id].rval = ada_sched_enable();
		break;
	case ADA_SCHED_SET_NAME:
		len = va_arg(ap, int);
		name = va_arg(ap, char *);
		api_test[id].rval = ada_sched_set_name(len, name);
		break;
	case ADA_SCHED_GET_INDEX:
		idx =   va_arg(ap, int);
		pname = va_arg(ap, char **);
		buf =   va_arg(ap, void *);
		psize =  va_arg(ap, size_t *);
		api_test[id].rval = ada_sched_get_index(idx, pname, buf, psize);
		break;
	case ADA_SCHED_SET_INDEX:
		idx =   va_arg(ap, unsigned int);
		buf =   va_arg(ap, void *);
		len =   va_arg(ap, unsigned int);
		api_test[id].rval = ada_sched_set_index(idx, buf, len);
		break;
	case ADA_OTA_REGISTER:
		idx =   va_arg(ap, unsigned int);
		pot =   va_arg(ap, struct ada_ota_ops *);
		ada_ota_register(idx, pot);
		api_test[id].rval = idx;
		break;
	case ADA_OTA_START:
		idx =   va_arg(ap, unsigned int);
		ada_ota_start(idx);
		api_test[id].rval = idx;
		break;
	case ADA_OTA_REPORT:
		idx =   va_arg(ap, unsigned int);
		len =   va_arg(ap, unsigned int);
		ada_ota_report(idx, len);
		api_test[id].rval = idx;
		break;
	case ADA_LOG_INFO:
		fmt = va_arg(ap, char *);
		name = va_arg(ap, char *);
		log_put_mod(LOG_MOD_DEFAULT, fmt, name);
		api_test[id].rval = 0;
		break;
	case ADA_LOG_PUT:
		fmt = va_arg(ap, char *);
		name = va_arg(ap, char *);
		log_put(fmt, name);
		api_test[id].rval = 0;
		break;
	default:
		demo_log(LOG_ERR  "Unknown test id %d", id);
		id = 0;
		break;
	}

	api_test[id].cnt++;
	if (id != ADA_SPROP_DEST_MASK || api_test[id].cnt % 600 == 0) {
		if (api_test[id].rval < 0 && api_test[id].rval != -7) {
			demo_log(LOG_ERR  "TC%d %s %d (%d) <%s>",
				api_test[id].tc, api_test[id].name,
				api_test[id].rval, api_test[id].cnt,
				ada_err_string(api_test[id].rval));
		} else {
			demo_log(LOG_DEBUG "TC%d %s %d (%d) <%s>",
				api_test[id].tc, api_test[id].name,
				api_test[id].rval, api_test[id].cnt,
				ada_err_string(api_test[id].rval));
		}
	}

	va_end(ap);

	return api_test[id].rval;
}

/*
 * CLI commands <test-show>
 */
void test_show_cmd(int argc, char **argv)
{
	int err = 0;

	if (argc < 2 || argc > 2) {
		err++;
	} else {
		if (!strcmp(argv[1], "res")) {
			test_show_res_cmd(argc, argv);
		} else if (!strcmp(argv[1], "sys")) {
			test_show_sys_cmd(argc, argv);
		} else if (!strcmp(argv[1], "prop")) {
			test_show_prop_cmd(argc, argv);
		} else {
			err++;
		}
	}

	if (err) {
		demo_log(LOG_ERR "Usage: %s %s", argv[0], test_show_help);
	}
}

/*
 * CLI command <test-show res> - show test results
 */
void test_show_res_cmd(int argc, char **argv)
{
	struct ada_test_api *atp = api_test;
	const char *emsg;

	while (atp->name != NULL) {
		emsg = (atp->rval == API_NOT_TESTED) ?
			"not tested" : ada_err_string(atp->rval);
		printcli("C%d %28s -> %3d (%6d) <%s>\r\n",
			atp->tc, atp->name, atp->rval, atp->cnt, emsg);
		atp++;
	}
}

/*
 * CLI command <test-show sys> - show device settings
 */
void test_show_sys_cmd(int argc, char **argv)
{
	enum conf_token token[CONF_PATH_MAX];
	int ntoken;
	const char *str_val;
	char *fmt = "%17s = %s\r\n";

	printcli(fmt, "model", conf_sys_model);

	str_val = NULL;
	ntoken = conf_str_to_tokens("id/dev_id", token, ARRAY_LEN(token));
	if (ntoken > 0) {
		str_val = conf_get_str(token, ntoken);
	}
	if (str_val) {
		printcli(fmt, "serial", str_val);
	} else {
		printcli(fmt, "serial", "");
	}

	str_val = NULL;
	ntoken = conf_str_to_tokens("oem/oem", token, ARRAY_LEN(token));
	if (ntoken > 0) {
		str_val = conf_get_str(token, ntoken);
	}
	if (str_val) {
		printcli(fmt, "oem", str_val);
	} else {
		printcli(fmt, "oem", "");
	}

	str_val = NULL;
	ntoken = conf_str_to_tokens("oem/model", token, ARRAY_LEN(token));
	if (ntoken > 0) {
		str_val = conf_get_str(token, ntoken);
	}
	if (str_val) {
		printcli(fmt, "oem_model", str_val);
	} else {
		printcli(fmt, "oem_model", "");
	}

	printcli(fmt, "version", adap_conf_sw_build());
	printcli(fmt, "version_demo", demo_host_version);
}

/*
 * CLI command <test-show prop> - show properies
 */
void test_show_prop_cmd(int argc, char **argv)
{
	int i;
	const char *fmt;

	for (i = 0; i < demo_props_num; i++) {
		switch (demo_props[i].type) {
		case ATLV_BOOL:
		case ATLV_INT:
		case ATLV_CENTS:
			fmt = "%17s = %d\r\n";
			printcli(fmt, demo_props[i].name,
				*(int *)demo_props[i].val);
			break;
		default:
			fmt = "%17s = %s\r\n";
			printcli(fmt, demo_props[i].name,
				demo_props[i].val);
			break;
		}
	}
}

/*
 * CLI commands <test-prop>
 */
void test_prop_cmd(int argc, char **argv)
{
	int err = 0;

	if (argc < 3 || argc > 4) {
		err++;
	} else {
		if (!strcmp(argv[1], "get")) {
			test_prop_get_cmd(argc, argv);
		} else if (!strcmp(argv[1], "set")) {
			test_prop_set_cmd(argc, argv);
		} else if (!strcmp(argv[1], "show")) {
			test_prop_show_cmd(argc, argv);
		} else {
			err++;
		}
	}
	if (err) {
		demo_log(LOG_ERR "Usage: %s %s", argv[0], test_prop_help);
	}
}

/*
 * CLI command <test-prop get> - property get
 */
void test_prop_get_cmd(int argc, char **argv)
{
	struct ada_sprop *asp;

	asp = sprop_find_by_name(argv[2]);
	if (asp) {
		asp->get(asp, asp->val, asp->val_len);
	} else {
		demo_log(LOG_ERR "Property %s not found", argv[2]);
	}
}

/*
 * CLI command <test-prop set> - property set
 */
void test_prop_set_cmd(int argc, char **argv)
{
	struct ada_sprop *asp;
	int val;
	int rv;

	asp = sprop_find_by_name(argv[2]);
	if (asp) {
		if (asp->set) {
			switch (asp->type) {
			case ATLV_BOOL:
				rv = sscanf(argv[3], "%u", &val);
				if (rv == 1) {
					val %= 256;
					asp->set(asp, &val, sizeof(val));
				} else {
					/* cstyle ignore width */
					demo_log(LOG_ERR "Bad bool val %s",
					    argv[3]);
					/* cstyle default */
				}
				break;
			case ATLV_INT:
			case ATLV_UINT:
			case ATLV_CENTS:
				rv = sscanf(argv[3], "%d", &val);
				if (rv == 1) {
					asp->set(asp, &val, sizeof(val));
				} else {
					/* cstyle ignore width */
					demo_log(LOG_ERR "Bad int val %s",
					    argv[3]);
					/* cstyle default */
				}
				break;
			default:
				asp->set(asp, argv[3], strlen(argv[3]));
				break;
			}
		} else {
			if (!strcmp(asp->name, "Blue_button")) {
					rv = sscanf(argv[3], "%d", &val);
					if (rv != 1) {
						/* cstyle ignore width */
						demo_log(LOG_ERR
						"Bad bool val %s", argv[3]);
						/* cstyle default */
					}
			} else {
				demo_log(LOG_ERR "%s is not an input property",
				    argv[2]);
			}
		}
	} else {
		demo_log(LOG_ERR "Property %s not found", argv[2]);
	}
}

/*
 * CLI command <test-prop show> - property show
 */
void test_prop_show_cmd(int argc, char **argv)
{
	const char *fmt;
	struct ada_sprop *asp;

	fmt = "%17s = %d\r\n";
	asp = sprop_find_by_name(argv[2]);
	if (asp) {
		switch (asp->type) {
		case ATLV_BOOL:
		case ATLV_INT:
		case ATLV_CENTS:
			printcli(fmt, asp->name, *(int *)asp->val);
			break;
		case ATLV_UINT:
			printcli(fmt, asp->name, *(int *)asp->val);
			break;
		default:
			printcli(fmt, asp->name, asp->val);
			break;
		}
	} else {
		demo_log(LOG_ERR "Property %s not found", argv[2]);
	}
}

/*
 * Simple Property Manager find and send property by name
 */
struct ada_sprop *sprop_find_by_name(const char *name)
{
	int i;
	struct ada_sprop *asp = NULL;

	for (i = 0; i < demo_props_num; i++) {
		if (!strcmp(name, demo_props[i].name)) {
			asp = &demo_props[i];
		}
	}
	return asp;
}

void sched_show(int idx)
{
	enum ada_err err;
	char *name;
	char buf[256];
	size_t len = 256;
	unsigned int i, j = 0;

	for (i = 0; i < 5; i++)	{
		if (i == idx || idx < 0) {
			err = ada_api_call(ADA_SCHED_GET_INDEX,
				i, &name, buf, &len);
			if (j == 0 && len != 0) {
				err = ada_api_call(ADA_SCHED_SET_INDEX,
					(i + 1)%5, buf, len);
				j++;
			}
			log_put(LOG_INFO "%s: set %s (%d) to size %d err %d\n",
				__func__, name, i, len, err);
		}
	}
}

/*
 * Logging for test component
 */
void test_log(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_put_va(MOD_LOG_TEST, fmt, args);
	va_end(args);
}
