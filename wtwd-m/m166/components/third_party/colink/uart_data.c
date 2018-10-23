#include <stdio.h>
#include <string.h>
#include "osal.h"
#include "hsuart/drv_hsuart.h"
#include "wdt/drv_wdt.h"
#include "colink_type.h"
#include "colink_define.h"
#include "colink_global.h"
#include "cJSON.h"

#define uart_printf(...)

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

/////////////////////////////////////
extern int colink_save_deviceid(char *deviceid, int size);
extern int colink_load_deviceid(char *deviceid, int size);
extern int colink_deviceid_sha256(ColinkDevice *pdev, char digest_hex[65]);

static int macaddress_conver(unsigned char mac[6], char *macstr)
{
	int i;
	char *endptr;

	for(i=0; i<6; i++)//d0:27:00:07:40:2c
	{
		mac[i] = (unsigned char)strtol(macstr, &endptr, 16);
		if(*endptr != ':') break;
		macstr = endptr + 1;
	}
	//printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	if(i == 5) return 0;
	return -1;
}

//工具发送：AT+SEND=<十进制字符串表示的数据长度>\r\n
//设备返回：>\r\n
//工具发送：<JSON格式的数据内容>
//"deviceid":"1000003192",
//"factory_apikey":"76eaabdc-5157-4d7a-a619-1794afe8120d",
//"sta_mac":"d0:27:00:00:60:74",
//"sap_mac":"d0:27:00:00:60:75",
//"device_model":"ITA-GZ1-GL"
//设备返回，成功：<16进制小写字母表示的64字节的sha256校验值>\r\nOK\r\n
//其中，校验内容为 deviceid值 + factory_apikey值 + sta_mac值 + sap_mac值 + device_model值 

const char *uart_request_s  = "AT+SEND=";
const char *uart_response_s = ">\r\n";
const char *uart_return_ok  = "OK\r\n";
const char *uart_format_str = "\r\nERROR FORMAT:fields lost\r\nOK\r\n";
const char *uart_iofail_str = "\r\nFLASH FAILED: IO ERROR\r\nOK\r\n";
const char *uart_exist_str  = "\r\nFLASH FAILED: FLASHED ALREADY\r\nOK\r\n";
extern ColinkDevice colink_dev;

//cammand low 16bits, param high 16bits
uint32_t colink_dl_check(ColinkDevice *pdev)
{
	char ch, state, buffer[65];
	uint32_t n, off, start_systick;

	state = 0;
	off = 0;
	start_systick = OS_GetSysTick();
	while(OS_GetSysTick() < start_systick + 10000) //10-second
	{
		off += uart_data_read_data((uint8_t*)buffer+off, 8-off);
		if(off == 8)
		{
			if(memcmp(buffer, uart_request_s, 8) == 0)
			{
				state = 1;
				break;
			}
		}
		for(off=1; off<8; off++)
		{
			if(buffer[off] == 'A') break;
		}
		if(off < 8) memcpy(buffer, buffer+off, 8-off);
		off = 8 - off;

		OS_MsDelay(100);
	}
	if(state == 0) return 0;

	off = 0;
	start_systick = OS_GetSysTick();
	while(OS_GetSysTick() < start_systick + 200)
	{
		n = uart_data_read_data((uint8_t*)&ch, 1);
		if(n)
		{
			buffer[off++] = ch;
			if(ch == '\n')
			{
				state = 2;
				off = 0;
				n = atoi(buffer);
				uart_data_send_data((uint8_t*)uart_response_s, 3);
				break;
			}
		}

		OS_MsDelay(5);
	}
	if(state != 2 || n < 100) return 0;

	char *pbuf = OS_MemAlloc(n+1);
	if(pbuf)
	{
		off = 0;
		start_systick = OS_GetSysTick();
		while(OS_GetSysTick() < start_systick + 500)
		{
			off += uart_data_read_data((uint8_t*)pbuf+off, n-off);
			if(off == n)
			{
				state = 3;
				break;
			}
			OS_MsDelay(5);
		}
	}
	if(state != 3)
	{
		if(pbuf) OS_MemFree(pbuf);
		return 0;
	}
	pbuf[n] = 0;

	/*if(colink_load_deviceid(NULL, 0) >= 0)
	{
		if(pbuf) OS_MemFree(pbuf);
		uart_data_send_data((uint8_t*)uart_exist_str, strlen(uart_exist_str));
		return 0;
	}*/

	cJSON *json_root;
	cJSON *json_temp_p;

	json_root = cJSON_Parse(pbuf);
	if (!json_root)
	{
		uart_printf("[%s]parse json failed", pbuf);
		goto exit1;
	}

	json_temp_p = cJSON_GetObjectItem(json_root, "deviceid");
	if(!json_temp_p) goto exit2;
	strcpy(pdev->deviceid, json_temp_p->valuestring);

	json_temp_p = cJSON_GetObjectItem(json_root, "factory_apikey");
	if(!json_temp_p) goto exit2;
	strcpy(pdev->apikey, json_temp_p->valuestring);

	json_temp_p = cJSON_GetObjectItem(json_root, "sta_mac");
	if(!json_temp_p) goto exit2;
	macaddress_conver(pdev->sta_mac, json_temp_p->valuestring);

	json_temp_p = cJSON_GetObjectItem(json_root, "sap_mac");
	if(!json_temp_p) goto exit2;
	macaddress_conver(pdev->sap_mac, json_temp_p->valuestring);

	json_temp_p = cJSON_GetObjectItem(json_root, "device_model");
	if(!json_temp_p) goto exit2;
	strcpy(pdev->model, json_temp_p->valuestring);

	if(colink_deviceid_sha256(pdev, buffer)) goto exit2;
	printf("sha256:[%s]\n", buffer);

	cJSON_Delete(json_root);
	OS_MemFree(pbuf);

	uart_data_send_data((uint8_t*)buffer, 64);
	uart_data_send_data((uint8_t*)&uart_response_s[1], 2);
	uart_data_send_data((uint8_t*)uart_return_ok, 4);
	return 1;

exit2:
	cJSON_Delete(json_root);
exit1:
	OS_MemFree(pbuf);
	uart_data_send_data((uint8_t*)uart_format_str, strlen(uart_format_str));
	return 0;
}

void dl_deviceid_task(void *pdata)
{
	ColinkDevice dev;

	if(colink_dl_check(&dev))
	{
		if(colink_save_deviceid((char*)&colink_dev, sizeof(ColinkDevice)) == sizeof(ColinkDevice))
		{
			if(coLinkGetDeviceMode() != DEVICE_MODE_START)
			{
				drv_wdt_init();
				drv_wdt_enable(SYS_WDT, 100);
			}
			else memcpy(&colink_dev, &dev, sizeof(ColinkDevice));
		}
		else uart_data_send_data((uint8_t*)uart_iofail_str, strlen(uart_iofail_str));
	}
	uart_data_free();
	OS_TaskDelete(NULL);
}

void colink_dl_deviceid_start(void)
{
	//19200，1起始位，8数据位，无校验位，1停止位
	uart_data_init(19200, HSUART_WLS_8, HSUART_STB_1, HSUART_PARITY_DISABLE, 0x200);
	OS_TaskCreate(dl_deviceid_task, "dl_deviceid", 512, NULL, 2, NULL);
}

