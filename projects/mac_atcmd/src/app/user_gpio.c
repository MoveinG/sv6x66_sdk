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


static OsTimer key_led_timer = NULL;


void key_led_handler(void)
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
			if(get_device_mode() == DUT_AP) set_device_mode(DUT_STA);
			else set_device_mode(DUT_AP);
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
			if(get_device_mode() == DUT_AP)
			{
				if(timer_count % 3 == 0)
				{
					//printf("Device enters AP mode\r\n");
					led_s = !led_s;
					drv_gpio_set_logic(DEVICE_LED, led_s);
				}

			}
			else 
			{
				if(timer_count % 15 == 0)
				{
					//printf("Device enters STA mode\r\n");
					led_s = !led_s;
					drv_gpio_set_logic(DEVICE_LED, led_s);	
				}

			}
			break;
		case KEY_SHORT:
			break;
		default:
			break;

	}
	if(get_device_mode() != DEVICE_START) return;
	if(get_connect_server_status() == true)
	{
		drv_gpio_set_logic(DEVICE_LED, 1);	
	}
	else
	{
		if(timer_count % 1 == 0)   
		{
			if((timer_count >= 4) && (timer_count < 20))
			{
				drv_gpio_set_logic(DEVICE_LED, 0);
				led_s = 0;
			}
			else if(timer_count >= 20)
			{
				timer_count = 0; 
			}
			else
			{
				led_s = !led_s;
				drv_gpio_set_logic(DEVICE_LED, led_s);
				
			}
				 
		}
	}

}



void Keyled_Init(void)
{
	drv_gpio_set_mode(DEVICE_KEY, 0);
	drv_gpio_set_dir(DEVICE_KEY, 0);
	drv_gpio_set_pull(DEVICE_KEY, GPIO_PULL_UP);

	drv_gpio_set_mode(DEVICE_LED, 0);
	drv_gpio_set_dir(DEVICE_LED, 1);
	drv_gpio_set_logic(DEVICE_LED, 0);

	if(OS_TimerCreate(&key_led_timer, 100, (unsigned char)1, NULL, (OsTimerHandler)key_led_handler) != OS_SUCCESS)
	{
		printf("create KEY_LED timer failed\r\n");
	}
	OS_TimerStart(key_led_timer);
	
}


static void user_key_led_task(void *arg)
{
	Keyled_Init();
	while(1)
	{
		 if (get_device_mode() == DUT_STA)
		 {
		 	DUT_wifi_start(DUT_STA);
			set_device_mode(DUT_STA);
		 }
		 else if (get_device_mode() == DUT_AP)
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


