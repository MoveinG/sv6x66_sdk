#include "colink_global.h"


static CoLinkDeviceMode dev_mode = DEVICE_MODE_INVALID;

CoLinkFlashParam colink_flash_param;

void coLinkSetDeviceMode(CoLinkDeviceMode mode)
{
    dev_mode = mode;
}

CoLinkDeviceMode coLinkGetDeviceMode(void)
{
    return dev_mode;
}


