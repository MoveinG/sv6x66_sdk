/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */

/*
 * Ayla device agent demo of a simple lights and buttons evaluation board
 * using the "simple" property manager.
 *
 * The property names are chosen to be compatible with the Ayla Control
 * App.  E.g., the LED property is Blue_LED even though the color is yellow.
 * Button1 sends the Blue_button property, even though the button is white.
 */
#include <stdlib.h>
#include <al/al_cli.h>
#include <al/al_os_thread.h>
#include <ada/ada.h>
#include <demo/demo.h>
#include "osal.h"

#define board_button_1() 0
#define board_button_2() 0
#define board_button_pressed(pin) 0
#define board_led_1() 0
#define led_on(pin)
#define led_off(pin)

#define BUILD_PROGNAME	"ayla_ledevb_demo"
#define BUILD_VERSION	"1.3"
#define BUILD_STRING	BUILD_VERSION " " __DATE__ " " __TIME__

static struct al_thread *demo_thread;

int conf_connected;

const char mod_sw_build[] = BUILD_STRING;
const char mod_sw_version[] = BUILD_PROGNAME " " BUILD_STRING;

static unsigned blue_button;
static u8 blue_led;
static u8 green_led;
static int value_led = 50;
static int input;
static int output;
static int decimal_in;
static int decimal_out;
static char version[] = BUILD_PROGNAME " " BUILD_STRING;
static char cmd_buf[TLV_MAX_STR_LEN + 1];
static char demo_host_version[] = "1.0.0.1";	/* property template version */

static enum ada_err demo_led_set(struct ada_sprop *, const void *, size_t);
static enum ada_err demo_int_set(struct ada_sprop *, const void *, size_t);
static enum ada_err demo_cmd_set(struct ada_sprop *, const void *, size_t);
static enum ada_err demo_led_val_set(struct ada_sprop *sprop,
				const void *buf, size_t len);
static void set_led(void)
{
}

static struct ada_sprop demo_props[] = {
	/*
	 * version properties
	 * oem_host_version is the template version and must be sent first.
	 */
	{ "oem_host_version", ATLV_UTF8,
	    demo_host_version, sizeof(demo_host_version),
	    ada_sprop_get_string, NULL},
	{ "version", ATLV_UTF8, &version[0], sizeof(version),
	    ada_sprop_get_string, NULL},
	/*
	 * boolean properties
	 */
	{ "Blue_button", ATLV_BOOL, &blue_button, sizeof(blue_button),
	    ada_sprop_get_bool, NULL},
	{ "Blue_LED", ATLV_BOOL, &blue_led, sizeof(blue_led),
	    ada_sprop_get_bool, demo_led_set },
	{ "Green_LED", ATLV_BOOL, &green_led, sizeof(green_led),
	    ada_sprop_get_bool, demo_led_set },
	{ "led_val", ATLV_INT, &value_led, sizeof(value_led),
	    ada_sprop_get_int, demo_led_val_set },
	/*
	 * string properties
	 */
	{ "cmd", ATLV_UTF8, &cmd_buf[0], sizeof(cmd_buf),
	    ada_sprop_get_string, demo_cmd_set },
	{ "log", ATLV_UTF8, &cmd_buf[0], sizeof(cmd_buf),
	    ada_sprop_get_string, NULL },
	/*
	 * integer properties
	 */
	{ "input", ATLV_INT, &input, sizeof(input),
	    ada_sprop_get_int, demo_int_set },
	{ "output", ATLV_INT, &output, sizeof(output),
	    ada_sprop_get_int, NULL },
	/*
	 * decimal properties
	 */
	{ "decimal_in", ATLV_CENTS, &decimal_in, sizeof(decimal_in),
	    ada_sprop_get_int, demo_int_set },
	{ "decimal_out", ATLV_CENTS, &decimal_out, sizeof(decimal_out),
	    ada_sprop_get_int, NULL },
};

static void prop_send_by_name(const char *name)
{
	enum ada_err err;

	err = ada_sprop_send_by_name(name);
	if (err) {
		log_info("demo: %s: send of %s: err %d",
		    __func__, name, err);
	}
}

/*
 * Demo set function for bool properties.
 */
static enum ada_err demo_led_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret = 0;

	ret = ada_sprop_set_bool(sprop, buf, len);
	if (ret) {
		return ret;
	}
	if (sprop->val == &blue_led) {
		set_led();
	}
	log_info("%s: %s set to %u",
	    __func__, sprop->name, *(u8 *)sprop->val);
	return AE_OK;
}

static enum ada_err demo_led_val_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret = 0;

	ret = ada_sprop_set_int(sprop, buf, len);
	if (ret) {
		return ret;
	}
	if (sprop->val == &value_led) {
		printf("###############  Set led val:%d\n",*(int *)sprop->val);
	}
	log_info("%s: %s set to %u",
	    __func__, sprop->name, *(u8 *)sprop->val);
	return AE_OK;
}

void change_led_val(int val){
	value_led = val;
	prop_send_by_name("led_val");
}
/*
 * Demo set function for integer and decimal properties.
 */
static enum ada_err demo_int_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret;

	ret = ada_sprop_set_int(sprop, buf, len);
	if (ret) {
		return ret;
	}

	if (sprop->val == &input) {
		log_info("%s: %s set to %d",
		    __func__, sprop->name, input);
		output = input;
		prop_send_by_name("output");
	} else if (sprop->val == &decimal_in) {
		log_info("%s: %s set to %d",
		    __func__, sprop->name, decimal_in);
		decimal_out = decimal_in;
		prop_send_by_name("decimal_out");
	} else {
		return AE_NOT_FOUND;
	}
	return AE_OK;
}

/*
 * Demo set function for string properties.
 */
static enum ada_err demo_cmd_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret;

	ret = ada_sprop_set_string(sprop, buf, len);
	if (ret) {
		return ret;
	}

	prop_send_by_name("log");
	log_info("%s: cloud set %s to \"%s\"",
	    __func__, sprop->name, cmd_buf);
	return AE_OK;
}

static void demo_ledevb_main(void *arg)
{
	struct {
		int gpio;
		int val;
	} button[2];
	int tmp, i;
	int link_led = 0;
	
	printf("start demo_ledevb_main");

	struct al_thread *thread = (struct al_thread *)arg;

	prop_send_by_name("oem_host_version");
	prop_send_by_name("version");
	prop_send_by_name("led_val");
	

	while (!al_os_thread_get_exit_flag(thread)) {
		if (!(ada_sprop_dest_mask & NODES_ADS)) {
			if (link_led) {
				link_led = 0;
				led_off(board_led_1());
			}
		} else if (!link_led) {
			conf_connected = 1;
			link_led = 1;
			led_on(board_led_1());
		}

		for (i = 0; i < ARRAY_LEN(button); i++) {
			tmp = board_button_pressed(button[i].gpio);
			if (tmp == button[i].val) {
				continue;
			}
			log_info("Button%d change to %d", i, tmp);
			button[i].val = tmp;
			if (i == 0) {
				blue_button = button[0].val;
				log_info("%s: blue_button to %d",
				    __func__, button[0].val);
				prop_send_by_name("Blue_button");
			}
			if (i == 1 && button[i].val == 1) {
				client_reg_window_start();
			}
		}

		al_os_thread_sleep(100);
	}
}

void demo_init(void)
{
	al_cli_set_prompt("PWB ledevb> ");
	if(AE_OK == ada_sprop_mgr_register("ledevb", demo_props, ARRAY_LEN(demo_props))){
		printf("ada_sprop_mgr_register success\n");
	}else{
		printf("ada_sprop_mgr_register false\n");
	}
	
	demo_thread = al_os_thread_create("ledevb_demo", NULL, 0,
	    al_os_thread_pri_normal, demo_ledevb_main, NULL);
	ASSERT(demo_thread);
}

#ifdef ADA_BUILD_FINAL
void demo_final(void)
{
	al_os_thread_terminate_with_status(demo_thread);
}
#endif
