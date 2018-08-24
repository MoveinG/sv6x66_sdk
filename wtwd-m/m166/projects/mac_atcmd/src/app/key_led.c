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
#define CONNECT_SET		2

#define SWITCH_CLOSE	0
#define SWITCH_OPEN		1

#define KEYLED_MSGLEN	10

static OsTimer led_flash_timer;
static u32 keydown_time;
static int led_status, pwr_status;

static OsMsgQ keyled_msgq;

extern void update_xlink_status(void);
extern void xlinkProcessStart(void);
extern void xlinkProcessEnd(void);

extern void colinkSwitchUpdate(void);
extern void colinkSettingStart(void);
extern void colinkProcessStart(void);

extern void esptouch_init(void);
extern void esptouch_stop(void);

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
	//msg_evt.MsgData = connect;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void TaskKeyLed(void *pdata)
{
	OsMsgQEntry msg_evt;
	bool smarting=false, cloud_task=false;
	uint8_t ssidlen=32, keylen=64;

	pwr_status = SWITCH_PWROFF;
	led_flash_timer = NULL;

	if(get_wifi_status() == 1) led_status = LED_LIGHT_ON;
	led_status = LED_LIGHT_OFF;

	KeyLed_Init();
	if(OS_TimerCreate(&led_flash_timer, LIGHT_FLASH1, (u8)FALSE, NULL, (OsTimerHandler)led_flash_handler) == OS_FAILED)
		return;

    if(OS_MsgQCreate(&keyled_msgq, KEYLED_MSGLEN) != OS_SUCCESS)
        return;

	printf("TaskKeyLed Init OK!\n");
	while(1)
	{
        if(OS_MsgQDequeue(keyled_msgq, &msg_evt, portMAX_DELAY) == OS_SUCCESS)
		{
			switch(msg_evt.MsgCmd) {
			case EVENT_DEV_KEY:
				if(msg_evt.MsgData == KEY_1LONG)
				{
					smarting = true;
					#if defined(CK_CLOUD_EN)
					system_param_delete();
					esptouch_stop();
					OS_TimerStart(led_flash_timer);
					esptouch_init();
					#endif

					#if defined(WT_CLOUD_EN)
					SmartConfig_Start();
					if(cloud_task) xlinkProcessEnd();
					cloud_task = false;
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
				if(smarting)
				{
					OS_TimerStop(led_flash_timer);
					smarting = false;
				}
				/*if(msg_evt.MsgData == CONNECT_SET)
				{
					show_wifi_status(1);

					#if defined(CK_CLOUD_EN)
					colinkSettingStart();
					#endif
				}*/
				if(msg_evt.MsgData == CONNECT_CON)
				{
					show_wifi_status(1);

					#if defined(WT_CLOUD_EN)
					if(!cloud_task) xlinkProcessStart();
					cloud_task = true;
					#endif

					#if defined(CK_CLOUD_EN)
					if(system_param_load(NULL, 0) == 0) colinkProcessStart();
					#endif
				}
				if(msg_evt.MsgData == CONNECT_DIS)
				{
					show_wifi_status(0);
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

