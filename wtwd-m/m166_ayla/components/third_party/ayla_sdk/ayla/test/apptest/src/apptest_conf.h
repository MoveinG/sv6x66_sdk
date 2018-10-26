/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_APPTEST_CONF_H__
#define __AYLA_APPTEST_CONF_H__

/*
 * ODM company name for factory log.
 */
#define DEMO_CONF_ODM  "ADA demo customer" /* replace with your company name */

/*
 * Module names used for PSM subsystem.
 */
#define ADA_CONF_MOD	"ada"		/* most items */

/*
 * string length limits
 */
#define CONF_PATH_STR_MAX	64	/* max len of conf variable name */
#define CONF_ADS_HOST_MAX (CONF_OEM_MAX + 1 + CONF_MODEL_MAX + 1 + 24)
					/* max ADS hostname length incl NUL */
#define ADA_PUB_KEY_LEN	400

extern char conf_sys_model[];
extern char conf_sys_serial[];

extern const char mod_sw_build[];
extern const char mod_sw_version[];
extern u8 conf_connected;

void client_conf_init(void);


void apptest_sched_conf_load(void);

#endif /* __AYLA_APPTEST_CONF_H__ */
