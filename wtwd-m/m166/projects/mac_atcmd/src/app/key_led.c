#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include "osal.h"
#include "gpio/drv_gpio.h"
#include "colink/include/colink_global.h"

#define DEVICE_WFLED	GPIO_13 //wifi
#define DEVICE_PWLED	GPIO_01 //power

#define DEVICE_KEY1		GPIO_00
//#define DEVICE_KEY2		GPIO_02

#define DEVICE_SWITCH1	GPIO_11
//#define DEVICE_SWITCH2	GPIO_11

#define LIGHT_FLASH1	250 //ms for smartconfig
#define LIGHT_FLASH2	1500 //ms for AP mode

#define LED_LIGHT_ON	0
#define LED_LIGHT_OFF	1

#define SWITCH_PWRON	1
#define SWITCH_PWROFF	0

#define KEY_DEBOUND		10
#define KEY_CHECK_TIME	100
#define KEY_LONG_VAL	50
//#define KEY_NORMAL_VAL	2

#define EVENT_DEV_KEY	1
#define EVENT_CONNECT	2
#define EVENT_SWITCH	3

#define KEY_KEY1		0x0001
#define KEY_KEY2		0x0002
#define KEY_1LONG		0x0100
//#define KEY_DOWN		0x1000
//#define KEY_TOUP		0x2000

#define CONNECT_DIS		0
#define CONNECT_CON		1
#define CONNECT_SET		2

#define SWITCH_CLOSE	0
#define SWITCH_OPEN		1

#define KEYLED_MSGLEN	10

static OsTimer led_flash_timer, key_check_timer;
static int led_status, pwr_status;

static OsMsgQ keyled_msgq;

extern void update_xlink_status(void);
extern void xlinkProcessStart(void);
extern void xlinkProcessEnd(void);

extern void colinkSwitchUpdate(void);
extern void colinkSettingStart(void);
extern void colinkProcessStart(void);
extern void enterSettingSelfAPMode(void);

extern void esptouch_init(void);
extern void esptouch_stop(void);

///////////////////////////////////////////
static void led_flash_handler(void)
{
	//printf("%s\n", __func__);
	OS_TimerStart(led_flash_timer);

	if(led_status == LED_LIGHT_OFF) led_status = LED_LIGHT_ON;
	else led_status = LED_LIGHT_OFF;

	drv_gpio_set_logic(DEVICE_WFLED, led_status);
}

static void key_check_handler(void)
{
    static int8_t last_key1=1, conut=0;
    uint8_t state;
	OsMsgQEntry msg_evt;

	state = drv_gpio_get_logic(DEVICE_KEY1);
	if(state == 0)
	{
		if(conut == KEY_LONG_VAL)
		{
			msg_evt.MsgCmd = EVENT_DEV_KEY;
			msg_evt.MsgData = KEY_1LONG;
			OS_MsgQEnqueue(keyled_msgq, &msg_evt);
		}
		conut++;
	}
	else
	{
		if(last_key1 == 0 && conut < KEY_LONG_VAL)
		{
			msg_evt.MsgCmd = EVENT_DEV_KEY;
			msg_evt.MsgData = KEY_KEY1;
			OS_MsgQEnqueue(keyled_msgq, &msg_evt);
		}
		conut = 0;
	}
	last_key1 = state;

#if defined(DEVICE_KEY2)
    static int8_t last_key2=1;

	state = drv_gpio_get_logic(DEVICE_KEY2);
	if(state != 0 && last_key2 == 0)
	{
		msg_evt.MsgCmd = EVENT_DEV_KEY;
		msg_evt.MsgData = KEY_KEY2;
		OS_MsgQEnqueue(keyled_msgq, &msg_evt);
	}
	last_key2 = state;
#endif
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
	//drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_FALLING_EDGE);
	//drv_gpio_intc_clear(DEVICE_KEY1);
	//drv_gpio_register_isr(DEVICE_KEY1, irq_key1_gpio_ipc);

#if defined(DEVICE_KEY2)
	drv_gpio_set_mode(DEVICE_KEY1, 0);
	drv_gpio_set_dir(DEVICE_KEY1, 0);
#endif

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
	uint32_t keydown_time=0;

	pwr_status = SWITCH_PWROFF;

	if(get_wifi_status() == 1) led_status = LED_LIGHT_ON;
	led_status = LED_LIGHT_OFF;

	led_flash_timer = NULL;
	if(OS_TimerCreate(&led_flash_timer, LIGHT_FLASH1, (u8)FALSE, NULL, (OsTimerHandler)led_flash_handler) == OS_FAILED)
		return;

	key_check_timer = NULL;
	if(OS_TimerCreate(&key_check_timer, KEY_CHECK_TIME, (u8)TRUE, NULL, (OsTimerHandler)key_check_handler) == OS_FAILED)
		return;

	if(OS_MsgQCreate(&keyled_msgq, KEYLED_MSGLEN) != OS_SUCCESS)
		return;

	KeyLed_Init();
	OS_TimerStart(key_check_timer);

	#if defined(CK_CLOUD_EN)
	coLinkSetDeviceMode(DEVICE_MODE_START); 
	#endif
	printf("TaskKeyLed Init OK!\n");
	while(1)
	{
        if(OS_MsgQDequeue(keyled_msgq, &msg_evt, portMAX_DELAY) == OS_SUCCESS)
		{
			switch(msg_evt.MsgCmd) {
			case EVENT_DEV_KEY:
				printf("EVENT_DEV_KEY=%x\n", msg_evt.MsgData);
				if(msg_evt.MsgData == KEY_1LONG)
				{
					if(smarting == true)
					{
						printf("to AP mode config\n");
						#if defined(CK_CLOUD_EN)
						OS_TimerSet(led_flash_timer, LIGHT_FLASH2, (u8)FALSE, NULL);
					    esptouch_stop();
						//smarting = false;
						enterSettingSelfAPMode();
						#endif
						break;
					}
					else printf("to smartconfig\n");

					smarting = true;
					#if defined(CK_CLOUD_EN)
					system_param_delete();
					esptouch_stop();
					OS_TimerStart(led_flash_timer);
				    coLinkSetDeviceMode(DEVICE_MODE_SETTING);
					esptouch_init();
					#endif

					#if defined(WT_CLOUD_EN)
					//SmartConfig_Start();
					joylink_stop();
					OS_TimerStart(led_flash_timer);
					joylink_init("WATERWORLDA15IOT");

					if(cloud_task) xlinkProcessEnd();
					cloud_task = false;
					#endif
				}
				if(msg_evt.MsgData == KEY_KEY1) 
				{
					if(smarting == true) break;

					Shift_Switch();
					#if defined(WT_CLOUD_EN)
					if(cloud_task) update_xlink_status();
					#endif
					#if defined(CK_CLOUD_EN)
					if(get_wifi_status() != 0) colinkSwitchUpdate();
					#endif
				}
				#if defined(DEVICE_KEY2)
				if(msg_evt.MsgData == KEY_KEY2)
				{
					Shift_Switch2();
				}
				#endif
				break;

			case EVENT_CONNECT:
				/*if(msg_evt.MsgData == CONNECT_SET)
				{
					show_wifi_status(1);

					#if defined(CK_CLOUD_EN)
					colinkSettingStart();
					#endif
				}*/
				if(msg_evt.MsgData == CONNECT_CON)
				{
					if(smarting)
					{
						OS_TimerStop(led_flash_timer);
						smarting = false;
					}
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
	OS_TimerDelete(key_check_timer);
	OS_TimerDelete(led_flash_timer);
	OS_TaskDelete(NULL);
}

