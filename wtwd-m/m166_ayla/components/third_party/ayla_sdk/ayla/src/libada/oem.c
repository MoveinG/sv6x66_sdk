/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <ada/ada.h>

void oem_conf_init(void)
{
	ada_conf_register(NULL, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_UTF8, &ada_conf.oem, sizeof(ada_conf.oem),
	    NULL, 2, CT_oem, CT_oem);

	ada_conf_register(NULL, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_UTF8, &ada_conf.model, sizeof(ada_conf.model),
	    NULL, 2, CT_oem, CT_model);

	ada_conf_register(NULL, NULL, CONF_REG_FLAG_READONLY,
	    ATLV_FILE,  &ada_conf.oemkey, sizeof(ada_conf.oemkey),
	    NULL, 2, CT_oem, CT_key);
}
