/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <al/al_cli.h>
#include <al/al_os_thread.h>
#include <ada/ada.h>
#include <ada/prop.h>
#include <ada/prop_mgr.h>
#include "apptest_demo.h"
#include "apptest_conf.h"
#include "apptest.h"

#define BUILD_PROGNAME	"ayla_apptest_demo"
#define BUILD_VERSION	"1.3"
#define BUILD_STRING	BUILD_VERSION " " __DATE__ " " __TIME__

static struct al_thread *demo_thread;

const char mod_sw_build[] = BUILD_STRING;
const char mod_sw_version[] = BUILD_PROGNAME " " BUILD_STRING;

static unsigned blue_button;
static u8 blue_led;
static u8 green_led;
static int input;
static int output;
static unsigned uinput;
static unsigned uoutput;
static int decimal_in;
static int decimal_out;
static char version[] = BUILD_PROGNAME " " BUILD_STRING;
static char cmd_buf[TLV_MAX_STR_LEN + 1];
char demo_host_version[] = "1.0-pwb";	/* property template version */

static enum ada_err apptest_demo_led_set(struct ada_sprop *,
	const void *, size_t);
static enum ada_err apptest_demo_int_set(struct ada_sprop *,
	const void *, size_t);
static enum ada_err apptest_demo_cmd_set(struct ada_sprop *,
	const void *, size_t);
static enum ada_err apptest_prop_mgr_set(const char *name,
			enum ayla_tlv_type type,
			const void *val, size_t val_len,
			size_t *offset, u8 source, void *set_arg);
static enum ada_err apptest_prop_mgr_get(const char *name,
			enum ada_err (*get_cb)(struct prop *,
				void *, enum ada_err),
			void *arg);
static void apptest_prop_mgr_send_done(enum prop_cb_status status,
	u8 fail_mask, void *cb_arg);
static void apptest_prop_mgr_connect_sts(u8 mask);

/*
 * Virtual Test Button structure
 */
static struct TestBut apptest_but[] = {
	{NULL, 0, -1},
	{NULL, 0, -1},
	{NULL, 0, -1},
	{NULL},
};

struct ada_sprop demo_props[] = {
	/*
	 * version properties
	 * oem_host_version is the template version and must be sent first.
	 */
	{ "oem_host_version", ATLV_UTF8,
	    demo_host_version, sizeof(demo_host_version),
	    api_sprop_get_string, NULL},
	{ "version", ATLV_UTF8, &version[0], sizeof(version),
	    api_sprop_get_string, NULL},
	/*
	 * boolean properties
	 */
	{ "Blue_button", ATLV_BOOL, &blue_button, sizeof(blue_button),
	    api_sprop_get_bool, NULL},
	{ "Blue_LED", ATLV_BOOL, &blue_led, sizeof(blue_led),
	    api_sprop_get_bool, apptest_demo_led_set },
	{ "Green_LED", ATLV_BOOL, &green_led, sizeof(green_led),
	    api_sprop_get_bool, apptest_demo_led_set },
	/*
	 * string properties
	 */
	{ "cmd", ATLV_UTF8, &cmd_buf[0], sizeof(cmd_buf),
	    api_sprop_get_string, apptest_demo_cmd_set },
	{ "log", ATLV_UTF8, &cmd_buf[0], sizeof(cmd_buf),
	    api_sprop_get_string, NULL },
	/*
	 * integer properties
	 */
	{ "input", ATLV_INT, &input, sizeof(input),
	    api_sprop_get_int, apptest_demo_int_set },
	{ "output", ATLV_INT, &output, sizeof(output),
	    api_sprop_get_int, NULL },

	/*
	 * unsigned integer properties
	 */
	{ "uinput", ATLV_UINT, &uinput, sizeof(uinput),
	    api_sprop_get_uint, apptest_demo_int_set },
	{ "uoutput", ATLV_UINT, &uoutput, sizeof(uoutput),
	    api_sprop_get_uint, NULL },

	/*
	 * decimal properties
	 */
	{ "decimal_in", ATLV_CENTS, &decimal_in, sizeof(decimal_in),
	    api_sprop_get_int, apptest_demo_int_set },
	{ "decimal_out", ATLV_CENTS, &decimal_out, sizeof(decimal_out),
	    api_sprop_get_int, NULL },
};

int demo_props_num = ARRAY_LEN(demo_props);

/*
 * Primitive Property Manager
 */
static u8 apptest_prop_mgr_dst_mask;

static unsigned outlet_pmgr;
static unsigned uinput_pmgr;
static unsigned uoutput_pmgr;

static struct prop apptest_prop_mgr_props[] = {
	{.name = "outlet_pmgr", .type = ATLV_BOOL,
		.len = sizeof(outlet_pmgr), .val = &outlet_pmgr},
	{.name = "uinput_pmgr",  .type = ATLV_UINT,
		.len = sizeof(uinput_pmgr),  .val = &uinput_pmgr},
	{.name = "uoutput_pmgr", .type = ATLV_UINT,
		.len = sizeof(uoutput_pmgr), .val = &uoutput_pmgr},
	{ NULL },
};

static struct prop_mgr apptest_prop_mgr = {
	.name           = "apptest_mgr",
	.prop_recv      = apptest_prop_mgr_set,
	.get_val        = apptest_prop_mgr_get,
	.send_done      = apptest_prop_mgr_send_done,
	.connect_status = apptest_prop_mgr_connect_sts
};

/*
 * Primitive Property Manager functions
 */
 /*
 * Simple Property Manager find and send property by name
 */
static struct prop *apptest_prop_find_by_name(const char *name)
{
	struct prop *tp = apptest_prop_mgr_props;

	while (!tp->name) {
		if (!strcmp(tp->name, name)) {
			break;
		}
		tp++;
	}

	if (!tp->name) {
		log_info("property <%s> not found",
			name);
	}

	return tp;
}

static enum ada_err apptest_prop_mgr_set(const char *name,
			enum ayla_tlv_type type,
			const void *val, size_t val_len,
			size_t *offset, u8 source, void *set_arg)
{
	struct prop *tp = apptest_prop_mgr_props;
	enum ada_err rv = AE_OK;

	while (tp->name) {
		if (!strcmp(name, tp->name)) {
			if (type != tp->type) {
				log_err(LOG_MOD_DEFAULT,
					"wrong type for %s - %d",
					name, type);
				rv = AE_INVAL_VAL;
				break;
			}
			switch (type) {
			case ATLV_BOOL:
			{
				(*(u8 *)tp->val) = (*(u8 *)val) != 0;
				break;
			}
			case ATLV_UINT:
			{
				(*(unsigned *)tp->val) = *(unsigned *)val;
				log_info("%s set to %d", name,
					(*(unsigned *)tp->val));
				break;
			}
			default:
			{
				break;
			}
			}
			if (!strcmp(name, "outlet_pmgr")) {
				outlet_pmgr = (*(u8 *)tp->val) != 0;
				log_info("%s set to %d", name,
					outlet_pmgr);
			}
			break;
		}
		tp++;
	}

	if (!tp->name) {
		log_info("property <%s> not found",
			name);
		rv = AE_NOT_FOUND;
		return rv;
	}

	log_info("prop mgr set property %s",
				name);

	ada_api_call(ADA_PROP_RECV, rv);

	return rv;
}

static enum ada_err apptest_prop_mgr_get(const char *name,
			enum ada_err (*get_cb)(struct prop *,
				void *, enum ada_err),
			void *arg)
{
	struct prop *tp = apptest_prop_mgr_props;
	enum ada_err rv = AE_OK;

	while (tp->name) {
		if (!strcmp(name, tp->name)) {
			break;
		}
		tp++;
	}
	if (!tp->name) {
		log_info("property <%s> not found",
			name);
		rv = AE_NOT_FOUND;
		return rv;
	}

	log_info("prop mgr get property %s",
				name);

	return (rv == AE_OK) ? get_cb(tp, arg, AE_OK) : AE_NOT_FOUND;
}

static void apptest_prop_mgr_send_done(enum prop_cb_status status,
	u8 fail_mask, void *cb_arg)
{
	struct prop *tp = cb_arg;

	log_info("prop mgr send done %s stat:%d fail_mask:%x",
		tp->name, status, fail_mask);
	ada_api_call(ADA_PROP_SEND_DONE, status);
}

static void apptest_prop_mgr_connect_sts(u8 mask)
{
	ada_api_call(ADA_PROP_MGR_READY, &apptest_prop_mgr);
	apptest_prop_mgr_dst_mask = mask;
	if (mask & NODES_ADS) {
		log_info("prop mgr connectivity restored, mask %x", mask);
	} else {
		log_err(LOG_MOD_DEFAULT,
			"prop mgr connectivity lost, mask %x", mask);
	}
	ada_api_call(ADA_PROP_CONNECT_STATUS, mask);
}

/*
 * Board buttons initialization
 */
static void apptest_but_init()
{
	apptest_but[0].name = "Blue_button";
	apptest_but[0].val = 0;
	blue_button = apptest_but[0].val;
	apptest_but[1].name = "Client_reg";
	apptest_but[1].val = 0;
	apptest_but[2].name = "outlet_pmgr";
	apptest_but[2].val = 0;
	outlet_pmgr = apptest_but[2].val;
}

/*
 * Board button find by name
 */
static struct TestBut *apptest_but_find(const char *name)
{
	struct TestBut *tbp = apptest_but;

	while (tbp->name) {
		if (!strcmp(tbp->name, name)) {
			break;
		}
		tbp++;
	}
	if (!tbp->name) {
		tbp = NULL;
	}
	return tbp;
}

/*
 * Board button has changed
 */
static int apptest_but_changed(const char *name)
{
	struct TestBut *tbp;
	int tmp;
	int rv = 0;

	tbp = apptest_but_find(name);
	if (tbp) {
		if (tbp->ctrl < 0) {
			tmp = tbp->val;
		} else {
			tmp = tbp->ctrl;
		}
		if (tmp != tbp->val) {
			tbp->val = tmp;
			rv++;
			log_info("Button %s changed to %d", tbp->name,
				tbp->val);
		}
	}
	return rv;
}

/*
 * Board button control switch from board (-1) to virtual (0, 1)
 */
static void apptest_but_ctrl(const char *name, int val)
{
	struct TestBut *tbp;

	tbp = apptest_but_find(name);
	if (tbp) {
		tbp->ctrl = val;
		if (val < 0) {
			log_info("%s <- %d switched to %d, button control",
				tbp->name, val, tbp->ctrl);
		} else {
			log_info("%s <- %d switched to %d, virtual control",
				tbp->name, val, tbp->ctrl);
		}
	}
}


static void apptest_prop_send_by_name(const char *name)
{
	enum ada_err err;

	if (!strcmp(name, "input") || !strcmp(name, "output")) {
		err = ada_api_call(ADA_SPROP_SEND, sprop_find_by_name(name));
	} else {
		err = ada_api_call(ADA_SPROP_SEND_BY_NAME, name);
	}
	if (err) {
		log_info("demo: send of %s: err %d",
		    name, err);
	}
}

/*
 * Demo set function for bool properties.
 */
static enum ada_err apptest_demo_led_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret = 0;

	ret = ada_api_call(ADA_SPROP_SET_BOOL, sprop, buf, len);
	if (ret) {
		return ret;
	}

	log_info("%s set to %u",
	    sprop->name, *(u8 *)sprop->val);
	return AE_OK;
}

/*
 * Demo set function for integer and decimal properties.
 */
static enum ada_err apptest_demo_int_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret;

	ret = ada_api_call((sprop->type == ATLV_UINT) ?
		ADA_SPROP_SET_UINT : ADA_SPROP_SET_INT, sprop, buf, len);
	if (ret) {
		return ret;
	}

	if (sprop->val == &input) {
		log_info("%s set to %d",
		    sprop->name, input);
		output = input;
		apptest_prop_send_by_name("output");
	} else if (sprop->val == &uinput) {
		log_info("%s set to %d",
		    sprop->name, uinput);
		uoutput = uinput;
		apptest_prop_send_by_name("uoutput");
	} else if (sprop->val == &decimal_in) {
		log_info("%s set to %d",
		    sprop->name, decimal_in);
		decimal_out = decimal_in;
		apptest_prop_send_by_name("decimal_out");
	} else {
		return AE_NOT_FOUND;
	}
	return AE_OK;
}

/*
 * Demo set function for string properties.
 */
static enum ada_err apptest_demo_cmd_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret;

	ret = ada_api_call(ADA_SPROP_SET_STRING, sprop, buf, len);
	if (ret) {
		return ret;
	}

	apptest_prop_send_by_name("log");
	log_info("cloud set %s to \"%s\"",
	    sprop->name, cmd_buf);
	return AE_OK;
}

static void apptest_demo_main(struct al_thread *thread, void *arg)
{
	int link_led = 0;
	int fsm = 0;
	struct TestBut *tbp;
	struct prop *tp;

	apptest_but_init();

	apptest_prop_send_by_name("oem_host_version");
	apptest_prop_send_by_name("version");

	while (!al_os_thread_get_exit_flag(thread)) {
		if (!(ada_api_call(ADA_SPROP_DEST_MASK) & NODES_ADS)) {
			if (link_led) {
				link_led = 0;
			}
		} else if (!link_led) {
			conf_connected = 1;
			link_led = 1;
			switch (fsm) {
			case 0:
			{
				ada_api_call(ADA_LOG_INFO,
					"virtual button control");
				ada_api_call(ADA_PROP_MGR_REQUEST,
						NULL);
				apptest_but_ctrl("Blue_button", 1);
				apptest_but_ctrl("Client_reg", 1);
				apptest_but_ctrl("outlet_pmgr", 1);
				fsm++;
				break;
			}
			default:
			{
				break;
			}
			}
		}

		/*
		 * Buttons processing
		 */
		tbp = apptest_but;
		while (tbp->name) {
			if (apptest_but_changed(tbp->name) == 0) {
				tbp++;
				continue;
			}
			ada_api_call(ADA_LOG_PUT,
				"process button %s value change,"\
				"ctrl %d, val %d",
				tbp->name, tbp->ctrl, tbp->val);
			if (!strcmp(tbp->name, "Blue_button")) {
				blue_button = tbp->val;
				apptest_prop_send_by_name(tbp->name);
			} else if (!strcmp(tbp->name, "Client_reg")) {
				if (tbp->val) {
					ada_api_call(\
					ADA_CLIENT_REG_WINDOW_START);
				}
			} else if (!strcmp(tbp->name, "outlet_pmgr")) {
				tp = apptest_prop_find_by_name(tbp->name);
				if (tp) {
					outlet_pmgr = tbp->val;
					ada_api_call(ADA_PROP_MGR_SEND,
						&apptest_prop_mgr, tp,
						apptest_prop_mgr_dst_mask,
						tp);
				}
			}
			apptest_but_ctrl(tbp->name, -1);
			tbp++;
		}

		al_os_thread_sleep(100);
	}
}

void apptest_demo_init(void)
{
	al_cli_set_prompt("PWB apptest> ");
	ada_api_call(ADA_SPROP_MGR_REGISTER, "apptest",
		demo_props, ARRAY_LEN(demo_props));
	ada_api_call(ADA_PROP_MGR_REGISTER, &apptest_prop_mgr);
	ada_api_call(ADA_PROP_MGR_READY, &apptest_prop_mgr);

	demo_thread = al_os_thread_create("apptest_demo", NULL, 0,
	    al_os_thread_pri_normal, apptest_demo_main, NULL);
	ASSERT(demo_thread);
}

#ifdef ADA_BUILD_FINAL
void apptest_demo_final(void)
{
	al_os_thread_terminate_with_status(demo_thread);
}
#endif
