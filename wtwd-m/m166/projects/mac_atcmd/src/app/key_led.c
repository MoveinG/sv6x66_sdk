#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include "osal.h"
#include "gpio/drv_gpio.h"

#define DEVICE_WFLED	GPIO_01 //wifi
#define DEVICE_PWLED	GPIO_00 //power

#define DEVICE_KEY1		GPIO_13
//#define DEVICE_KEY2		GPIO_02

#define DEVICE_SWITCH1	GPIO_12
//#define DEVICE_SWITCH2	GPIO_11

#define LIGHT_FLASH1	250 //ms for smartconfig
#define LIGHT_FLASH2	1500 //ms for AP mode

#define LED_LIGHT_ON	0
#define LED_LIGHT_OFF	1

#define SWITCH_PWRON	1
#define SWITCH_PWROFF	0

#define KEY_DEBOUND		10
#define KEY_LONG_TIME	2000
//#define KEY_LONG_VAL	1
//#define KEY_NORMAL_VAL	2

#define EVENT_DEV_KEY	1
#define EVENT_CONNECT	2
#define EVENT_SWITCH	3

#define KEY_1LONG		1
#define KEY_KEY1		2
#define KEY_KEY2		3

#define CONNECT_DIS		0
#define CONNECT_CON		1

#define SWITCH_CLOSE	0
#define SWITCH_OPEN		1

#define KEYLED_MSGLEN	10

static OsTimer led_flash_timer;
static u32 keydown_time;
static int led_status, pwr_status;

//static OsSemaphore key_led_sem;
static OsMsgQ keyled_msgq;

//for power_on auto_reconnect
//static uint8_t cur_ssid[32+1], cur_key[64+1], cur_mac[6];

extern void update_xlink_status(void);
extern void xlinkProcessStart(void);
extern void xlinkProcessEnd(void);

extern void colinkSwitchUpdate(void);
extern void colinkSettingStart(void);
extern void colinkProcessStart(void);

extern void user_main(void);
///////////////////////////////////////////
static void led_flash_handler(void)
{
	//printf("%s\n", __func__);
	OS_TimerStart(led_flash_timer);

	if(led_status == LED_LIGHT_OFF) led_status = LED_LIGHT_ON;
	else led_status = LED_LIGHT_OFF;

	/*if(get_wifi_status() == 1)
	{
		led_status = LED_LIGHT_ON;
		OS_TimerStop(led_flash_timer);
		led_flash_timer = NULL;
	}*/
	drv_gpio_set_logic(DEVICE_WFLED, led_status);
}

static int SmartConfig_Start(void)
{
    printf("%s\n", __func__);
	
	joylink_stop();
	//if(led_flash_timer != NULL)
	//{
	//	OS_TimerStop(led_flash_timer);
	//	led_flash_timer = NULL;
	//}
	OS_TimerStart(led_flash_timer);

	joylink_init("WATERWORLDA15IOT");
}

static void Shift_Switch(void)
{
	int led_pwr;

	OS_EnterCritical();
	if(pwr_status == SWITCH_PWROFF)
	{
		pwr_status = SWITCH_PWRON;
		led_pwr = LED_LIGHT_ON;
	}
	else
	{
		pwr_status = SWITCH_PWROFF;
		led_pwr = LED_LIGHT_OFF;
	}
	drv_gpio_set_logic(DEVICE_SWITCH1, pwr_status);
	drv_gpio_set_logic(DEVICE_PWLED, led_pwr);

	OS_ExitCritical();
}

static void irq_key1_gpio_ipc(uint32_t irq_num)
{
	int8_t level;
	u32 key_time;
	OsMsgQEntry msg_evt;

	printf("%s\n", __func__);

	drv_gpio_intc_clear(DEVICE_KEY1);
	level = drv_gpio_get_logic(DEVICE_KEY1);

	if(level == 0)
	{
		keydown_time = os_tick2ms(OS_GetSysTick());
		drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_RISING_EDGE);

		printf("keydown_time=%d\n", keydown_time);
	}
	else
	{
		key_time = os_tick2ms(OS_GetSysTick()) - keydown_time;
		drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_FALLING_EDGE);

		if(key_time > KEY_DEBOUND)
		{
			msg_evt.MsgCmd = EVENT_DEV_KEY;
			if(key_time > KEY_LONG_TIME) msg_evt.MsgData = KEY_1LONG;
			else msg_evt.MsgData = KEY_KEY1;

			//OS_SemSignal(key_led_sem);
	        OS_MsgQEnqueue(keyled_msgq, &msg_evt);
		}
		printf("key_time=%d, event=%d\n", key_time, msg_evt.MsgData);
	}
}

static void show_wifi_status(int connect)
{
	OS_EnterCritical();
	if(connect) led_status = LED_LIGHT_ON;
	else led_status = LED_LIGHT_OFF;
	OS_ExitCritical();

	drv_gpio_set_logic(DEVICE_WFLED, led_status);
}

static void KeyLed_Init(void)
{
	drv_gpio_set_mode(DEVICE_KEY1, 0);
	drv_gpio_set_dir(DEVICE_KEY1, 0);
	drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_FALLING_EDGE);
	//drv_gpio_intc_clear(DEVICE_KEY1);
	drv_gpio_register_isr(DEVICE_KEY1, irq_key1_gpio_ipc);

	drv_gpio_set_mode(DEVICE_WFLED, 0);
	drv_gpio_set_dir(DEVICE_WFLED, 1);
	drv_gpio_set_logic(DEVICE_WFLED, led_status);

	drv_gpio_set_mode(DEVICE_PWLED, 0);
	drv_gpio_set_dir(DEVICE_PWLED, 1);
	drv_gpio_set_logic(DEVICE_PWLED, LED_LIGHT_OFF);

	drv_gpio_set_mode(DEVICE_SWITCH1, 0);
	drv_gpio_set_dir(DEVICE_SWITCH1, 1);
	drv_gpio_set_logic(DEVICE_SWITCH1, SWITCH_PWROFF);

#if defined(DEVICE_SWITCH2)
	drv_gpio_set_mode(DEVICE_SWITCH2, 0);
	drv_gpio_set_dir(DEVICE_SWITCH2, 1);
	drv_gpio_set_logic(DEVICE_SWITCH2, SWITCH_PWROFF);
#endif
}

#if 0
static int check_set_wifi_config(void)
{   
    uint8_t ssid[33], ssidlen = 32, key[65], keylen = 64, mac[6];

    if(get_wifi_config(ssid, &ssidlen, key, &keylen, mac, 6) == 0)
    {
        ssid[ssidlen] = 0;
        key[keylen] = 0;

		printf("ssid(%s=%s), key(%s=%s), mac(%02x %02x %02x %02x %02x %02x=%02x %02x %02x %02x %02x %02x)\n", 
				ssid, cur_ssid, key, cur_key, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
				cur_mac[0], cur_mac[1], cur_mac[2], cur_mac[3], cur_mac[4], cur_mac[5]);

		if(strcmp(ssid, cur_ssid) || strcmp(key, cur_key) || memcmp(mac, cur_mac, sizeof(mac)))
		{
			recordAP();
			memcpy(cur_ssid, ssid, sizeof(cur_ssid));
			memcpy(cur_key, key, sizeof(cur_key));
			memcpy(cur_mac, mac, sizeof(cur_mac));
			return 1;
		}
    }
	return 0;
}
#endif

int get_Switch_status(void)
{
	return (pwr_status == SWITCH_PWRON) ? 1 : 0;
}

void Ctrl_Switch_cb(int open)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_SWITCH;
	if(open) msg_evt.MsgData = SWITCH_OPEN;
	else msg_evt.MsgData = SWITCH_CLOSE;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void wifi_status_cb(int connect)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_CONNECT;
	if(connect) msg_evt.MsgData = CONNECT_CON;
	else msg_evt.MsgData = CONNECT_DIS;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void TaskKeyLed(void *pdata)
{
	OsMsgQEntry msg_evt;
	bool smarting=false, cloud_task=false;
	uint8_t ssidlen=32, keylen=64;

	//key_value = 0;
	pwr_status = SWITCH_PWROFF;
	led_flash_timer = NULL;

	//get_wifi_config(cur_ssid, &ssidlen, cur_key, &keylen, cur_mac, 6);

	if(get_wifi_status() == 1) led_status = LED_LIGHT_ON;
	led_status = LED_LIGHT_OFF;

	KeyLed_Init();
	if(OS_TimerCreate(&led_flash_timer, LIGHT_FLASH1, (u8)FALSE, NULL, (OsTimerHandler)led_flash_handler) == OS_FAILED)
		return;

	//if(OS_SemInit(&key_led_sem, 1, 0) == OS_FAILED)
    if(OS_MsgQCreate(&keyled_msgq, KEYLED_MSGLEN) != OS_SUCCESS)
        return;

	printf("TaskKeyLed Init OK!\n");
	while(1)
	{
		//OS_SemWait(key_led_sem, portMAX_DELAY);
		//if(key_value == KEY_LONG_VAL) SmartConfig_Start();
		//if(key_value == KEY_NORMAL_VAL) Shift_Switch();
        if(OS_MsgQDequeue(keyled_msgq, &msg_evt, portMAX_DELAY) == OS_SUCCESS)
		{
			switch(msg_evt.MsgCmd) {
			case EVENT_DEV_KEY:
				if(msg_evt.MsgData == KEY_1LONG)
				{
					#if defined(TY_CLOUD_EN)
					user_main();
					#else
					smarting = true;
					SmartConfig_Start();
					#if defined(WT_CLOUD_EN)
					if(cloud_task) xlinkProcessEnd();
					cloud_task = false;
					#endif
					#endif
				}
				if(msg_evt.MsgData == KEY_KEY1) 
				{
					Shift_Switch();
					#if defined(WT_CLOUD_EN)
					if(cloud_task) update_xlink_status();
					#endif
					#if defined(CK_CLOUD_EN)
					colinkSwitchUpdate();
					#endif
				}
				//if(msg_evt.MsgData == KEY_KEY2) Shift_Switch2();
				break;

			case EVENT_CONNECT:
				if(msg_evt.MsgData == CONNECT_CON)
				{
					if(smarting)
					{
						OS_TimerStop(led_flash_timer);
						smarting = false;
					}
					show_wifi_status(1);
					//check_set_wifi_config();
					#if defined(WT_CLOUD_EN)
					if(!cloud_task) xlinkProcessStart();
					cloud_task = true;
					#endif
					#if defined(CK_CLOUD_EN)
					/*if(system_param_load(&ssidlen, 1) != 0) colinkSettingStart();
					else */colinkProcessStart();
					#endif
				}
				if(msg_evt.MsgData == CONNECT_DIS)
				{
					if(smarting == false) show_wifi_status(0);
				}
				break;

			case EVENT_SWITCH:
				if((msg_evt.MsgData == SWITCH_OPEN && pwr_status == SWITCH_PWROFF)
					|| (msg_evt.MsgData == SWITCH_CLOSE && pwr_status == SWITCH_PWRON))
				{
					Shift_Switch();
				}
				break;
			default:
				break;
			}
		}
	}

	OS_MsgQDelete(keyled_msgq);
	//OS_SemDelete(key_led_sem);
	OS_TimerDelete(led_flash_timer);
	OS_TaskDelete(NULL);
}

