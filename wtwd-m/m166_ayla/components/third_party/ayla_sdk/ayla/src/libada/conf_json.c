/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jsmn.h>
#include <ayla/conf.h>
#include <ayla/http.h>
#include <ayla/jsmn_get.h>
#include <ayla/log.h>
#include <al/al_clock.h>
#include <al/al_os_reboot.h>
#include <ada/err.h>
#include <ada/client.h>
#include <ada/server_req.h>

#define CONFIG_JSON_PUT_TOKENS	40
#define CONFIG_NAME_LEN 100
#define CONFIG_VAL_LEN 100

void conf_json_get(struct server_req *req)
{
}

static int conf_json_put_config(jsmn_parser *parser, jsmntok_t *obj, void *arg)
{
	char name[CONFIG_NAME_LEN];
	char val[CONFIG_VAL_LEN];
	long lval;

	if (jsmn_get_string(parser, obj, "name", name, sizeof(name)) <= 0) {
		server_log(LOG_WARN "conf_json_put no name");
		return -1;
	}

	/*
	 * Get value as a string, which works even for integers.
	 */
	if (jsmn_get_string(parser, obj, "val", val, sizeof(val)) <= 0) {
		return -1;
	}

	/*
	 * Special case for timezone.  The service is giving us minutes east
	 * of UTC, but we use minutes west.
	 */
	if (!strcmp(name, "sys/timezone")) {
		if (jsmn_get_long(parser, obj, "val", &lval)) {
			return -1;
		}
		snprintf(val, sizeof(val), "%ld", -lval);
	}

	conf_log(LOG_DEBUG "%s %s=%s", __func__, name, val);
	return conf_set(name, val);
}

static void conf_json_put_close(struct server_req *req)
{
	int commit_ret = (int)req->prov_impl;
	if (commit_ret & CONF_COMMIT_RET_REBOOT) {
		al_os_reboot();
		ASSERT_NOTREACHED();
	}
}

void conf_json_put(struct server_req *req)
{
	jsmn_parser parser;
	jsmntok_t tokens[CONFIG_JSON_PUT_TOKENS];
	jsmntok_t *config;
	jsmnerr_t err;

	conf_commit_start();
	jsmn_init_parser(&parser, req->post_data, tokens, ARRAY_LEN(tokens));
	err = jsmn_parse(&parser);
	if (err != JSMN_SUCCESS) {
		server_log(LOG_WARN "%s jsmn err %d", __func__, err);
		goto inval;
	}
	config = jsmn_get_val(&parser, NULL, "config");
	if (!config) {
		server_log(LOG_WARN "%s no config array", __func__);
		goto inval;
	}
	if (jsmn_array_iterate(&parser, config, conf_json_put_config, NULL)) {
		server_log(LOG_WARN "%s failed", __func__);
		goto inval;
	}
	req->prov_impl = (void *)conf_commit(0, 1);
	req->close_cb = conf_json_put_close;
	server_put_status(req, HTTP_STATUS_NO_CONTENT);
	return;
inval:
	server_put_status(req, HTTP_STATUS_BAD_REQ);
	return;
}
