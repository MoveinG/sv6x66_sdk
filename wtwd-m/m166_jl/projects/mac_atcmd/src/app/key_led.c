#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include "osal.h"
#include "gpio/drv_gpio.h"
#include "wdt/drv_wdt.h"
#include "iotapi/wifi_api.h"

#if defined(CK_CLOUD_EN)
	#include "colink/include/colink_global.h"
	#include "colink/include/colink_setting.h"
	#include "colink/include/colink_profile.h"
	#include "colink/include/mytime.h"
#endif
#include "sysconf_api.h"


#define AUTO_TEST_WIFI_SSID       "EVL_8DB0839D"
#define AUTO_TEST_WIFI_PWD        "094FAFE8"

#define DEVICE_WFLED	GPIO_13 //wifi
#define DEVICE_PWLED	GPIO_01 //power

#define DEVICE_KEY1		GPIO_00
//#define DEVICE_KEY2		GPIO_02

#define DEVICE_SWITCH1	GPIO_11
//#define DEVICE_SWITCH2	GPIO_11

#define LIGHT_F_TIME	100 //100ms
#define LIGHT_F_COUNT	20 //20

#define DEVICE_STATION	0
#define DEVICE_SMART	1
#define DEVICE_AP_MODE	2

#define WIFI_DIS_CON	0
#define WIFI_CONNECT	1
#define WIFI_INTERNET	2

#define COLINK_OFFLINE	0
#define COLINK_ONLINE	1
#define COLINK_UN_REG	2
#define COLINK_UPGRADE	3

#define STATUS_NORMAL	0
#define STATUS_NO_WIFI	1
#define STATUS_NO_SER	2
#define STATUS_UN_REG	3
#define STATUS_UPGRADE	4
#define STATUS_SMART	5
#define STATUS_AP_MODE	6
#define STATUS_MAX_NUM	7

#define LED_LIGHT_ON	1
#define LED_LIGHT_OFF	0

#define SWITCH_PWRON	1
#define SWITCH_PWROFF	0

#define KEY_CHECK_TIME	100
#define KEY_LONG_VAL	50

#define EVENT_DEV_KEY	1
#define EVENT_CONNECT	2
#define EVENT_SWITCH	3
#define EVENT_01_SEC	4
#define EVENT_SW_TIMER	5
#define EVENT_UP_TIMER	6
#define EVENT_DEVSTATUS	7
#define EVENT_PING114	8
#define EVENT_BRIGHT	0x100
#define EVENT_FLHMODE	0x200

#define KEY_KEY1		0x0001
#define KEY_KEY2		0x0002
#define KEY_1LONG		0x0100

#define CONNECT_DIS		0
#define CONNECT_CON		1

#define SWITCH_CLOSE	0
#define SWITCH_OPEN		1

#define KEYLED_MSGLEN	10

static OsTimer led_flash_timer, key_check_timer, cycle_1sec_timer;
static bool pwr_status;
static unsigned char led_status, dev_status=0, wifi_status=0, colink_status=0;

static OsMsgQ keyled_msgq;
//static OsMutex kl_mutex;

extern void update_xlink_status(void);
extern void xlinkProcessStart(void);
extern void xlinkProcessEnd(void);

extern void colinkSwitchUpdate(void);
extern void colinkSettingStart(void);
extern void colinkProcessStart(void);
extern void enterSettingSelfAPMode(void);

extern void esptouch_init(void);
extern void esptouch_stop(void);

extern void joylink_init(char *key);
extern void joylink_stop(void);
extern void wifi_auto_connect_start(void);

extern void colink_dl_deviceid_start(void);

int testing_ssid_cmp(void);

///////////////////////////////////////////
static void cycle_1sec_handler(void)
{
	OsMsgQEntry msg_evt;
	msg_evt.MsgCmd = EVENT_01_SEC;
	msg_evt.MsgData = (void*)NULL;
	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

static short update_led_status(void)
{
	printf("device=%d, wifi=%d, colink=%d\n", dev_status, wifi_status, colink_status);

	if(dev_status == DEVICE_SMART) return STATUS_SMART;
	if(dev_status == DEVICE_AP_MODE) return STATUS_AP_MODE;
	//dev_status==DEVICE_STATION

	if(wifi_status == WIFI_DIS_CON) return STATUS_NO_WIFI;
	if(wifi_status == WIFI_CONNECT) return STATUS_NO_SER;
	//if(wifi_status == WIFI_INTERNET) return STATUS_NORMAL;

	if(colink_status == COLINK_UPGRADE) return STATUS_UPGRADE;
	if(colink_status == COLINK_UN_REG) return STATUS_UN_REG;
	if(colink_status == COLINK_ONLINE) return STATUS_NORMAL;
	if(colink_status == COLINK_OFFLINE) return STATUS_NO_SER;

	return STATUS_NORMAL;
}

static void led_flash_handler(void)
{
	const static unsigned int status[STATUS_MAX_NUM]={0xFFFFF,0x00001,0x00005,0x003FF,0x00015,0xFFCCC,0x33333};
	static short count, prev_status=-1;
	static gpio_logic_t led_prev=LED_LIGHT_OFF;

	if(prev_status != led_status)
	{
		printf("%d current status: %d\n", prev_status, led_status);
		count = 0;
		prev_status = led_status;
	}

	if(led_status < STATUS_MAX_NUM)
	{
		gpio_logic_t led_gpio;

		led_gpio = status[led_status] & (1 << count) ? LED_LIGHT_ON : LED_LIGHT_OFF;
		if(led_gpio != led_prev)
		{
			drv_gpio_set_logic(DEVICE_WFLED, led_gpio);
			led_prev = led_gpio;
		}
	}
	if(++count >= LIGHT_F_COUNT) count = 0;
}

static void key_check_handler(void)
{
	static char last_key1=1, conut=0;
	unsigned char state;
	OsMsgQEntry msg_evt;

	state = drv_gpio_get_logic(DEVICE_KEY1);
	if(state == 0)
	{
		if(conut == KEY_LONG_VAL)
		{
			msg_evt.MsgCmd = EVENT_DEV_KEY;
			msg_evt.MsgData = (void*)KEY_1LONG;
			OS_MsgQEnqueue(keyled_msgq, &msg_evt);
		}
		conut++;
	}
	else
	{
		if(last_key1 == 0 && conut < KEY_LONG_VAL)
		{
			msg_evt.MsgCmd = EVENT_DEV_KEY;
			msg_evt.MsgData = (void*)KEY_KEY1;
			OS_MsgQEnqueue(keyled_msgq, &msg_evt);
		}
		conut = 0;
	}
	last_key1 = state;

#if defined(DEVICE_KEY2)
	static char last_key2=1;

	state = drv_gpio_get_logic(DEVICE_KEY2);
	if(state != 0 && last_key2 == 0)
	{
		msg_evt.MsgCmd = EVENT_DEV_KEY;
		msg_evt.MsgData = (void*)KEY_KEY2;
		OS_MsgQEnqueue(keyled_msgq, &msg_evt);
	}
	last_key2 = state;
#endif
}

static void active_Switch(bool pwron)
{
	if(pwron) pwr_status = TRUE;
	else pwr_status = FALSE;

	drv_gpio_set_logic(DEVICE_SWITCH1, pwr_status ? SWITCH_PWRON : SWITCH_PWROFF);
	drv_gpio_set_logic(DEVICE_PWLED, pwr_status ? LED_LIGHT_ON : LED_LIGHT_OFF);
}

static void KeyLed_Init(void)
{
	drv_gpio_set_mode(DEVICE_KEY1, 0);
	drv_gpio_set_dir(DEVICE_KEY1, 0);
	drv_gpio_set_pull(DEVICE_KEY1, GPIO_PULL_UP);
	//drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_FALLING_EDGE);
	//drv_gpio_intc_clear(DEVICE_KEY1);
	//drv_gpio_register_isr(DEVICE_KEY1, irq_key1_gpio_ipc);

#if defined(DEVICE_KEY2)
	drv_gpio_set_mode(DEVICE_KEY1, 0);
	drv_gpio_set_dir(DEVICE_KEY1, 0);
#endif

	drv_gpio_set_mode(DEVICE_WFLED, 0);
	drv_gpio_set_dir(DEVICE_WFLED, 1);
	drv_gpio_set_logic(DEVICE_WFLED, LED_LIGHT_OFF);

	drv_gpio_set_mode(DEVICE_PWLED, 0);
	drv_gpio_set_dir(DEVICE_PWLED, 1);
	drv_gpio_set_logic(DEVICE_PWLED, LED_LIGHT_OFF);

	drv_gpio_set_mode(DEVICE_SWITCH1, 0);
	drv_gpio_set_dir(DEVICE_SWITCH1, 1);
	//drv_gpio_set_logic(DEVICE_SWITCH1, SWITCH_PWROFF);

#if defined(DEVICE_SWITCH2)
	drv_gpio_set_mode(DEVICE_SWITCH2, 0);
	drv_gpio_set_dir(DEVICE_SWITCH2, 1);
	drv_gpio_set_logic(DEVICE_SWITCH2, SWITCH_PWROFF);
#endif
}

#if defined(CK_CLOUD_EN)
static void exit_link_config(unsigned short state)
{
	if(state)
	{
		if(state == 1) esptouch_stop();
		if(state == 2) softap_exit();
		wifi_auto_connect_start();
	}
}
#endif

static void wifi_internet_ping(void)
{
	struct myParam {
		char* argv[24]; //same as MAX_ARGUMENT 24 at ping.c
		int argc;
	} param = {{"114.114.114.114", "-c", "2", "-s", "16", NULL}, 5};

	extern int ping(void*);
	ping(&param);
}

int get_Switch_status(void)
{
	return (pwr_status == SWITCH_PWRON) ? 1 : 0;
}

void Ctrl_Switch_cb(int open)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_SWITCH;
	if(open) msg_evt.MsgData = (void*)SWITCH_OPEN;
	else msg_evt.MsgData = (void*)SWITCH_CLOSE;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void Timer_Switch_cb(int open)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_SW_TIMER;
	if(open) msg_evt.MsgData = (void*)SWITCH_OPEN;
	else msg_evt.MsgData = (void*)SWITCH_CLOSE;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void Timer_update_time(void)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_UP_TIMER;
	msg_evt.MsgData = (void*)0;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void Ctrl_bright_cb(int bright)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_BRIGHT;
	msg_evt.MsgData = (void*)bright;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void Ctrl_flashmode_cb(int mode)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_FLHMODE;
	msg_evt.MsgData = (void*)mode;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void wifi_status_cb(int connect)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_CONNECT;
	if(connect) msg_evt.MsgData = (void*)CONNECT_CON;
	else msg_evt.MsgData = (void*)CONNECT_DIS;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

#if defined(CK_CLOUD_EN)
void DevStatus_Notify_cb(ColinkDevStatus status)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_DEVSTATUS;
	msg_evt.MsgData = (void*)status;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}
#endif

void Ping_Notify_cb(unsigned int ping, unsigned int recv)
{
	OsMsgQEntry msg_evt;

	msg_evt.MsgCmd = EVENT_PING114;
	if(ping != 0 && recv == 0) msg_evt.MsgData = (void*)0;
	else msg_evt.MsgData = (void*)1;

	OS_MsgQEnqueue(keyled_msgq, &msg_evt);
}

void TaskKeyLed(void *pdata)
{
	static unsigned int start_smart_t;
	#if defined(CK_CLOUD_EN)
	static CoLinkDeviceMode save_mode;
	#endif
	OsMsgQEntry msg_evt;
	#if defined(WT_CLOUD_EN)
	bool cloud_task=false;
	#endif
	char clear_count=9, sec_30=0;
	int value;

	if(get_wifi_status() == 1) led_status = STATUS_NO_SER;
	else led_status = STATUS_NO_WIFI;

	cycle_1sec_timer = NULL;
	if(OS_TimerCreate(&cycle_1sec_timer, 1000, (unsigned char)1, NULL, (OsTimerHandler)cycle_1sec_handler) != OS_SUCCESS)
		goto exit0;
	OS_TimerStart(cycle_1sec_timer);

	led_flash_timer = NULL;
	if(OS_TimerCreate(&led_flash_timer, LIGHT_F_TIME, (unsigned char)1, NULL, (OsTimerHandler)led_flash_handler) != OS_SUCCESS)
		goto exit1;

	key_check_timer = NULL;
	if(OS_TimerCreate(&key_check_timer, KEY_CHECK_TIME, (unsigned char)1, NULL, (OsTimerHandler)key_check_handler) != OS_SUCCESS)
		goto exit2;

	if(OS_MsgQCreate(&keyled_msgq, KEYLED_MSGLEN) != OS_SUCCESS)
		goto exit3;

	KeyLed_Init();
	active_Switch(get_switch_nvram());

	OS_TimerStart(led_flash_timer);
	OS_TimerStart(key_check_timer);

	#if defined(CK_CLOUD_EN)
	coLinkSetDeviceMode(DEVICE_MODE_START);
	colink_Init_Device();
	colink_dl_deviceid_start();

	value = get_poweron_number();
	printf("get_poweron_number=%d\n", value);
	if(value > 4)
	{
		OS_MsDelay(3000);
		msg_evt.MsgCmd = EVENT_DEV_KEY;
		msg_evt.MsgData = (void*)KEY_1LONG;
		OS_MsgQEnqueue(keyled_msgq, &msg_evt);
	}
	else 
		wifi_auto_connect_start();
	#endif

	drv_wdt_init();
	drv_wdt_enable(SYS_WDT, 20000);//20-second

	printf("TaskKeyLed Init OK!\n");
	while(1)
	{
        if(OS_MsgQDequeue(keyled_msgq, &msg_evt, portMAX_DELAY) == OS_SUCCESS)
		{
			switch(msg_evt.MsgCmd) {
			case EVENT_01_SEC:
				#if defined(CK_CLOUD_EN)
				if(clear_count > 0)
				{
					clear_count--;
					if(clear_count == 0) clear_poweron_number();
				}

				if(led_status == STATUS_SMART || led_status == STATUS_AP_MODE)
				{
					if(os_tick2ms(OS_GetSysTick()) > start_smart_t + 180000) //3-min
					{
						coLinkSetDeviceMode(save_mode);
						exit_link_config(dev_status);
						dev_status = DEVICE_STATION;
						wifi_status = WIFI_DIS_CON;
						led_status = update_led_status();
					}
				}
				#endif
				drv_wdt_kick(SYS_WDT);

				if(wifi_status != WIFI_DIS_CON)
				{
					if(sec_30 == 0) wifi_internet_ping();
					if(++sec_30 >= 30) sec_30 = 0;
				}
				break;

			case EVENT_DEV_KEY:
				printf("EVENT_DEV_KEY=%x\n", (int)msg_evt.MsgData);
				if(msg_evt.MsgData == (void*)KEY_1LONG)
				{
				    remove_wifi_config();
					#if defined(CK_CLOUD_EN)
					if(coLinkGetDeviceMode() == DEVICE_MODE_UPGRADE)
						break;

					if(dev_status == DEVICE_AP_MODE) break;
					if(dev_status == DEVICE_SMART)
					{
						printf("to AP mode config\n");
						start_smart_t = os_tick2ms(OS_GetSysTick());

						dev_status = DEVICE_AP_MODE;
						led_status = update_led_status();
						esptouch_stop();
						enterSettingSelfAPMode();
						break;
					}
					else
					{
						printf("to smartconfig\n");
						save_mode = coLinkGetDeviceMode();
						start_smart_t = os_tick2ms(OS_GetSysTick());
					}

					dev_status = DEVICE_SMART;
					led_status = update_led_status();
					esptouch_stop();
					coLinkSetDeviceMode(DEVICE_MODE_SETTING);
					esptouch_init();
					#endif

					#if defined(WT_CLOUD_EN)
					dev_status = DEVICE_SMART;
					led_status = update_led_status();
					joylink_stop();
					joylink_init("WATERWORLDA15IOT");

					if(cloud_task) xlinkProcessEnd();
					cloud_task = false;
					#endif
				}
				if(msg_evt.MsgData == (void*)KEY_KEY1)
				{
					#if defined(CK_CLOUD_EN)
					if(testing_ssid_cmp() == 0)
					{
					    int cnts = 6;
						while(cnts--)
						{
                            active_Switch(pwr_status ? 0 : 1);
							OS_MsDelay(200);
						}
					}
					else
					{
					    if(dev_status)
					    {
						    coLinkSetDeviceMode(save_mode);
						    exit_link_config(dev_status);
						    dev_status = DEVICE_STATION;
						    wifi_status = WIFI_DIS_CON;
						    led_status = update_led_status();
						    break;
					    }

					    if(coLinkGetDeviceMode() == DEVICE_MODE_UPGRADE)
						    break;

					    active_Switch(pwr_status ? 0 : 1);
					    set_switch_nvram(pwr_status);

					    colinkSwitchUpdate();
					}
					#endif

					#if defined(WT_CLOUD_EN)
					if(cloud_task) update_xlink_status();
					#endif
				}
				#if defined(DEVICE_KEY2)
				if(msg_evt.MsgData == (void*)KEY_KEY2)
				{
					active_Switch2();
				}
				#endif
				break;

			case EVENT_CONNECT:
				if(msg_evt.MsgData == (void*)CONNECT_CON)
				{
					if(dev_status != DEVICE_STATION) dev_status = DEVICE_STATION;
					wifi_status = WIFI_CONNECT;

					#if defined(WT_CLOUD_EN)
					if(!cloud_task) xlinkProcessStart();
					cloud_task = true;
					#endif

					#if defined(CK_CLOUD_EN)
					mytime_start();
					if(coLinkGetDeviceMode() != DEVICE_MODE_SETTING) colinkProcessStart();
					#endif
				}
				if(msg_evt.MsgData == (void*)CONNECT_DIS)
				{
					wifi_status = WIFI_DIS_CON;
				}

				led_status = update_led_status();
				break;

			#if defined(CK_CLOUD_EN)
			case EVENT_DEVSTATUS:
				value = (int)msg_evt.MsgData;
				if((ColinkDevStatus)value == DEVICE_UNREGISTERED)
				{
					colink_status = COLINK_UN_REG;
					colink_delete_timer();
				}
				if((ColinkDevStatus)value == DEVICE_ONLINE)
				{
					colink_status = COLINK_ONLINE;
					colinkSwitchUpdate();
				}
				if((ColinkDevStatus)value == DEVICE_OFFLINE)
				{
					colink_status = COLINK_OFFLINE;
				}
				if((ColinkDevStatus)value == 10) colink_status = COLINK_UPGRADE;
				if((ColinkDevStatus)value == 11) colink_status = COLINK_ONLINE;

				led_status = update_led_status();
				break;
			#endif

			case EVENT_PING114:
				value = (int)msg_evt.MsgData;
				if(1 == value)
				{
					if(wifi_status == WIFI_INTERNET) break;
					wifi_status = WIFI_INTERNET;
				}
				else
				{
					if(wifi_status == WIFI_CONNECT) break;
					wifi_status = WIFI_CONNECT;
				}
				led_status = update_led_status();
				break;

			case EVENT_SW_TIMER:
				if(led_status != STATUS_NORMAL) break;
			case EVENT_SWITCH:
				if((msg_evt.MsgData == (void*)SWITCH_OPEN && pwr_status == SWITCH_PWROFF)
					|| (msg_evt.MsgData == (void*)SWITCH_CLOSE && pwr_status == SWITCH_PWRON))
				{
					active_Switch(pwr_status ? 0 : 1);
					set_switch_nvram(pwr_status);

					#if defined(CK_CLOUD_EN)
					if(msg_evt.MsgCmd == EVENT_SW_TIMER) colinkSwitchUpdate();
					#endif
				}
				break;

			#if defined(CK_CLOUD_EN)
			case EVENT_UP_TIMER:
				if(mytime_update_delay()) mytime_clean_delay();
				break;

			case EVENT_BRIGHT:
				value = (int)msg_evt.MsgData;
				//if(value > 6 && value <= 100) maoxin_set_light(value / 6); //10-100 -> 1-16
				break;

			case EVENT_FLHMODE:
				value = (int)msg_evt.MsgData;
				//if(value == 2 || value == 3) maoxin_light_flash(value-1);
				break;
			#endif

			default:
				break;
			}
		}
	}
	#if defined(CK_CLOUD_EN)
	mytime_stop();
	#endif
	OS_MsgQDelete(keyled_msgq);
exit3:
	OS_TimerDelete(key_check_timer);
exit2:
	OS_TimerDelete(led_flash_timer);
exit1:
	OS_TimerDelete(cycle_1sec_timer);
exit0:
	OS_TaskDelete(NULL);
}
extern void atwificbfunc(WIFI_RSP *msg);
void first_auto_testing(void)
{
    int ret;

    ret = read_wifi_config();
	if(ret != 0)
	{
        //DUT_wifi_start(DUT_STA);
        wifi_connect_active (AUTO_TEST_WIFI_SSID, strlen(AUTO_TEST_WIFI_SSID), AUTO_TEST_WIFI_PWD, strlen(AUTO_TEST_WIFI_PWD), atwificbfunc);
	}
}

extern IEEE80211STATUS gwifistatus;
int testing_ssid_cmp(void)
{
    int ret = -1;
	
    if((strcmp(AUTO_TEST_WIFI_SSID,gwifistatus.connAP[0].ssid) == 0) && (gwifistatus.connAP[0].ssid_len == strlen(AUTO_TEST_WIFI_SSID)))
   	{
        ret = 0;
    }
	return ret;
}

