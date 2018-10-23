#include <stdio.h>
#include <string.h>
#include "osal.h"
#include "hsuart/drv_hsuart.h"

#define uart_printf printf

/***********************************************************
*************************micro define***********************
***********************************************************/
#define UARTR_ONCE_SIZE	32

typedef struct {
	uint32_t buf_len;
	uint8_t *buf;
	uint16_t in;
	uint16_t out;
} uart_data_s;

/***********************************************************
*************************variable define********************
***********************************************************/
static uart_data_s uart_data;

/***********************************************************
*************************function define********************
***********************************************************/
static void hsuart_isr(void);

/***********************************************************
*  Function: uart_data_init 
*  Input: badu[1,5000000], bits[0,3] is [5,8], parity[0,1,3,5,7], stop[0,1] is [1,2]
*  Output: none
*  Return: int
***********************************************************/
int uart_data_init(int32_t badu, HSUART_WLS_E bits, HSUART_STB_E stop, HSUART_PARITY_E parity, uint32_t bufsz)
{
	uart_printf("%s badu=%d, bits=%d, parity=%d, stop=%d, bufsz=%d\n", __func__, (int)badu, bits, parity, stop, (int)bufsz);

	if(bufsz == 0) return -2;

	if(uart_data.buf == NULL)
	{
		memset(&uart_data, 0, sizeof(uart_data_s));

		uart_data.buf = OS_MemAlloc(bufsz);
		if(uart_data.buf == NULL) return -3;

		uart_data.buf_len = bufsz;
		uart_printf("uart buf size : %d", (int)bufsz);
	}
	else return -1;

	drv_hsuart_init();
	drv_hsuart_sw_rst();
	drv_hsuart_set_fifo(HSUART_INT_RX_FIFO_TRIG_LV_16);
	drv_hsuart_set_hardware_flow_control (16, 24);

	int retval = drv_hsuart_set_format(badu, bits, stop, parity);
	if(retval != 0)
	{
		OS_MemFree(uart_data.buf);
		uart_data.buf = NULL;
		uart_printf("retval=%d\n", retval);
		return -1;
	}
	drv_hsuart_register_isr(HSUART_RX_DATA_READY_IE, hsuart_isr);

	return 0;
}

/***********************************************************
*  Function: uart_data_free 
*  Input:free uart
*  Output: none
*  Return: int
***********************************************************/
int uart_data_free(void)
{
	uart_printf("%s\n", __func__);

	if(uart_data.buf != NULL)
	{
		OS_MemFree(uart_data.buf);
		uart_data.buf = NULL;
	}
	uart_data.buf_len = 0;

	drv_hsuart_deinit();
	return 0;
}

/***********************************************************
*  Function: uart_data_send_data 
*  Input: data len
*  Output: none
*  Return: none
***********************************************************/
void uart_data_send_data(const uint8_t *data,const uint32_t len)
{
	uart_printf("%s data=0x%x, len=%d\n", __func__, (int)data, (int)len);

	if(NULL == data || 0 == len) return;

	drv_hsuart_write_fifo(data, len, HSUART_BLOCKING);
}

static uint32_t uart_data_read_data_size(void)
{
	uint32_t remain_buf_size=0;

	if(uart_data.in >= uart_data.out)
	{
		remain_buf_size = uart_data.in-uart_data.out;
	}
	else
	{
		remain_buf_size = uart_data.in + uart_data.buf_len - uart_data.out;
	}

	return remain_buf_size;
}

static void hsuart_isr(void)
{
	int retval;

	retval = uart_data.buf_len - uart_data.in - 1;
	if(retval > UARTR_ONCE_SIZE) retval = UARTR_ONCE_SIZE;

	retval = drv_hsuart_read_fifo(uart_data.buf+uart_data.in, retval, HSUART_NON_BLOCKING);
	if(retval >= 0)
	{
		uart_data.in += retval;
		if(uart_data.in >= uart_data.buf_len) uart_data.in = 0;
	}
	else uart_printf("hsuart_isr=%d\n", retval);
}

/***********************************************************
*  Function: uart_data_read_data 
*  Input: len->data buf len
*  Output: buf->read data buf
*  Return: actual read data size
***********************************************************/
uint32_t uart_data_read_data(uint8_t *buf, uint32_t len)
{
	uart_printf("%s buf=0x%x, len=%d\n", __func__, (int)buf, (int)len);

	if(NULL == buf || 0 == len) return 0;

	uint32_t actual_size = 0;
	uint32_t cur_num = uart_data_read_data_size();

	if(cur_num > uart_data.buf_len - 1)
		uart_printf("uart fifo is full! buf_zize:%d  len:%d", (int)cur_num, (int)len);

	if(len > cur_num) actual_size = cur_num;
	else actual_size = len;

	uart_printf("cur_num=%d, actual_size = %d\n", (int)cur_num, (int)actual_size);

	uint32_t i = 0;
	for(i = 0;i < actual_size;i++)
	{
		*buf++ = uart_data.buf[uart_data.out++];
		if(uart_data.out >= uart_data.buf_len) uart_data.out = 0;
	}

	return actual_size;
}

///////////////////////////////////////////
//1. A0 FA 01 00 A5;  开
//2. A0 FA 02 00 A5;  关
//3. A0 FA 03 XX A5;  XX 取值 1 ~ 16，表示16级亮度
//4. A0 FA 04 XX A5;  XX 取值 1 ~ 2， 1 表示 快闪，2 表示慢闪
#define UART_COMMAND_LEN	5
static const uint8_t light_common_str[] = "\xA0\xFA\x01\x00\xA5";

int maoxin_uart_init(uint32_t baud, uint32_t bufsize)
{
    return uart_data_init(baud, HSUART_WLS_8, HSUART_STB_1, HSUART_PARITY_DISABLE, bufsize);
}

void maoxin_light_switch(int open)
{
	uint8_t command_str[UART_COMMAND_LEN];

	memcpy((char*)command_str, light_common_str, UART_COMMAND_LEN);
	if(!open) command_str[2] = 2;
	uart_data_send_data(command_str, UART_COMMAND_LEN);
}

void maoxin_set_light(int level)
{
	uint8_t command_str[UART_COMMAND_LEN];

	memcpy((char*)command_str, light_common_str, UART_COMMAND_LEN);
	command_str[2] = 3;
	command_str[3] = (uint8_t)level;
	uart_data_send_data(command_str, UART_COMMAND_LEN);
}

void maoxin_light_flash(int value)
{
	uint8_t command_str[UART_COMMAND_LEN];

	memcpy((char*)command_str, light_common_str, UART_COMMAND_LEN);
	command_str[2] = 4;
	command_str[3] = (uint8_t)value;
	uart_data_send_data(command_str, UART_COMMAND_LEN);
}

//cammand low 16bits, param high 16bits
uint32_t maoxin_get_command(void)
{
	static uint8_t command_buffer[UART_COMMAND_LEN];
	static uint8_t off=0;

	off += uart_data_read_data(command_buffer+off, UART_COMMAND_LEN-off);
	if(off == UART_COMMAND_LEN)
	{
		if(command_buffer[0] == 0xA0)
		{
			if(command_buffer[1] == 0xFA && command_buffer[4] == 0xA5)
			{
				off = 0;
				return command_buffer[2] | (command_buffer[3] << 16);
			}
		}
		for(off=1; off<UART_COMMAND_LEN; off++)
		{
			if(command_buffer[off] == 0xA0) break;
		}
		if(off < UART_COMMAND_LEN) memcpy(command_buffer, command_buffer+off, UART_COMMAND_LEN-off);
		off = UART_COMMAND_LEN - off;
	}
	return 0;
}

