
#include <string.h>
#include "soc_types.h"
#include "sys/backtrace.h"
#include "sys/xip.h"
#include "fsal.h"
#include "osal.h"
#include "wifinetstack.h"
#include "idmanage/pbuf.h"
#include "security/drv_security.h"
#include "phy/drv_phy.h"
#include "soc_defs.h"
#include "ieee80211_mgmt.h"
#include "ieee80211_mac.h"
#include "sta_func.h"
#include "error.h"
#include "wifi_api.h"
#include "netstack.h"
#include "netstack_def.h"
#include "uart/drv_uart.h"
#include "rf/rf_api.h"
#include "gpio/drv_gpio.h"
#include "user_transport.h"
#include "user_gpio.h"


#define KEY_TIME    30
#define KEY_LONG 	1
#define KEY_SHORT 	2

#define DEVICE_KEY GPIO_00
#define DEVICE_LED GPIO_13

#define DEVICE_START 0


static OsTimer key_event_timer=NULL;
static char DeviceMode = DEVICE_START;



void key_event_handler(void)
{
	static char last_key1 = 1, conut = 0;
	unsigned char state;
	static uint8_t timer_count, led_s;
	static char key_status;

	timer_count++;
	state = drv_gpio_get_logic(DEVICE_KEY);
	if(state == 0)
	{
		if(conut == KEY_TIME)
		{
			key_status = KEY_LONG;
			DeviceMode = get_device_mode();
			printf("DeviceMode = %d \r\n",DeviceMode);
			if(DeviceMode == DUT_AP) DeviceMode = DUT_STA;
			else DeviceMode = DUT_AP;
			printf("DeviceMode2 = %d \r\n",DeviceMode);
			
		}
		conut++;
	}
	else
	{
		if(last_key1 == 0 && conut < KEY_TIME)
		{
			key_status = KEY_SHORT;
		}
		conut = 0;
	}
	last_key1 = state;
	
	switch(key_status)
	{
		case KEY_LONG:
			if(DeviceMode == DUT_AP)
			{
				if(timer_count % 3 == 0)
				{
					
					printf("Device enters AP mode\r\n");
					led_s = !led_s;
					drv_gpio_set_logic(DEVICE_LED, led_s);
				}

			}
			else 
			{
				if(timer_count % 15 == 0)
				{
					printf("Device enters STA mode\r\n");
					led_s = !led_s;
					drv_gpio_set_logic(DEVICE_LED, led_s);
				}

			}
			break;
		case KEY_SHORT:
			printf("Device exit\r\n");
			DeviceMode = DEVICE_START;
			drv_gpio_set_logic(DEVICE_LED, 0);
			break;
		default:
			break;

	}
	

}


void Keyled_Init(void)
{
	drv_gpio_set_mode(DEVICE_KEY, 0);
	drv_gpio_set_dir(DEVICE_KEY, 0);
	drv_gpio_set_pull(DEVICE_KEY, GPIO_PULL_UP);

	drv_gpio_set_mode(DEVICE_LED, 0);
	drv_gpio_set_dir(DEVICE_LED, 1);
	drv_gpio_set_logic(DEVICE_LED, 1);

	if(OS_TimerCreate(&key_event_timer, 100, (unsigned char)1, NULL, (OsTimerHandler)key_event_handler) != OS_SUCCESS)
	{
		printf("create KEY timer failed\r\n");
	}
	OS_TimerStart(key_event_timer);
}


static void user_key_led_task(void *arg)
{
	Keyled_Init();
	while(1)
	{
		 if (DeviceMode == DUT_STA)
		 {
		 	DUT_wifi_start(DUT_STA);
			set_device_mode(DUT_STA);
		 }
		 else if (DeviceMode == DUT_AP)
		 {
		 	DUT_wifi_start(DUT_AP);
			set_device_mode(DUT_AP);
			
		 }
		 OS_MsDelay(20);
	}
	vTaskDelete(NULL);
}



void key_led_task_create(void)
{
	OS_TaskCreate(user_key_led_task, "user_key_led_task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
}


