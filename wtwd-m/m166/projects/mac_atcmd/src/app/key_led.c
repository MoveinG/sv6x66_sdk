#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include "osal.h"
#include "gpio/drv_gpio.h"

#define DEVICE_WFLED	GPIO_01 //wifi
#define DEVICE_PWLED	GPIO_00 //power

#define DEVICE_KEY1		GPIO_13
#define DEVICE_KEY2		GPIO_02

#define DEVICE_SWITCH1	GPIO_12
#define DEVICE_SWITCH2	GPIO_11

#define LIGHT_FLASH1	250 //ms for smartconfig
#define LIGHT_FLASH2	1500 //ms for AP mode

#define LED_LIGHT_ON	0
#define LED_LIGHT_OFF	1

#define SWITCH_PWRON	1
#define SWITCH_PWROFF	0

#define KEY_DEBOUND		10
#define KEY_LONG_TIME	1000
#define KEY_LONG_VAL	1
#define KEY_NORMAL_VAL	2

static OsTimer led_flash_timer;
static OsSemaphore key_led_sem;
static u32 keydown_time;
static int key_value, led_status, pwr_status;

void led_flash_handler(void)
{
    //printf("%s\n", __func__);
    OS_TimerStart(led_flash_timer);

	if(led_status == LED_LIGHT_OFF) led_status = LED_LIGHT_ON;
	else led_status = LED_LIGHT_OFF;

	if(get_wifi_status() == 1)
	{
		led_status = LED_LIGHT_ON;
		OS_TimerStop(led_flash_timer);
		led_flash_timer = NULL;
	}
	drv_gpio_set_logic(DEVICE_WFLED, led_status);
}

int SmartConfig_Start(void)
{
    printf("%s\n", __func__);
	
	joylink_stop();
	if(led_flash_timer != NULL)
	{
		OS_TimerStop(led_flash_timer);
		led_flash_timer = NULL;
	}

	joylink_init("9CTGIVN6AEK77EGI");
	if(OS_TimerCreate(&led_flash_timer, LIGHT_FLASH1, (u8)FALSE, NULL, (OsTimerHandler)led_flash_handler) == OS_FAILED)
        return OS_FAILED;

	OS_TimerStart(led_flash_timer);
}

void Switch_Power(void)
{
	int led_pwr;

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
}

void irq_key1_gpio_ipc(uint32_t irq_num)
{
	int8_t level;
	u32 key_time;

    printf("%s\n", __func__);

	drv_gpio_intc_clear(DEVICE_KEY1);
	level = drv_gpio_get_logic(DEVICE_KEY1);

	if(level == 0)
	{
		keydown_time = OS_GetSysTick();
		drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_RISING_EDGE);

	    printf("keydown_time=%d\n", keydown_time);
	}
	else
	{
		key_time = OS_GetSysTick() - keydown_time;
		drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_FALLING_EDGE);

		if(key_time > KEY_DEBOUND)
		{
			if(key_time > KEY_LONG_TIME) key_value = KEY_LONG_VAL;
			else key_value = KEY_NORMAL_VAL;

			OS_SemSignal(key_led_sem);
		}
	    printf("key_time=%d, key_code=%d\n", key_time, key_value);
	}
}

void KeyLed_Init(void)
{
	drv_gpio_set_mode(DEVICE_KEY1, 0);
	drv_gpio_set_dir(DEVICE_KEY1, 0);
	drv_gpio_intc_trigger_mode(DEVICE_KEY1, GPIO_INTC_FALLING_EDGE);
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

void TaskKeyLed(void *pdata)
{
	key_value = 0;
	pwr_status = SWITCH_PWROFF;
	led_flash_timer = NULL;

	if(get_wifi_status() == 1) led_status = LED_LIGHT_ON;
	led_status = LED_LIGHT_OFF;

	KeyLed_Init();
	if (OS_SemInit(&key_led_sem, 1, 0) == OS_FAILED)
        return OS_FAILED;

	printf("TaskKeyLed Init OK!\n");
    while(1)
	{
        OS_SemWait(key_led_sem, portMAX_DELAY);
		if(key_value == KEY_LONG_VAL) SmartConfig_Start();
		if(key_value == KEY_NORMAL_VAL) Switch_Power();
    }
	OS_SemDelete(key_led_sem);
}

