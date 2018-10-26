/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_os_reboot.h>
#include "wdt/drv_wdt.h"

void al_os_reboot(void)
{
    drv_wdt_deinit();
	drv_wdt_init(); //reboot
	drv_wdt_enable(SYS_WDT, 100);
}
