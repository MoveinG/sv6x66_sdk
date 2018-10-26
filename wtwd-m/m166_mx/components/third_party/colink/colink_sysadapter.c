#include "colink_define.h"
#include "colink_sysadapter.h"
#include "osal.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include "uart/drv_uart.h"
#include "lwip/def.h"
#include "iotapi/wifi_api.h"

#define PRINTF_BUFFER_SIZE (128) //becase printf(libc_path.c) is 128
static char printf_buffer[PRINTF_BUFFER_SIZE];

int32_t colinkGettime(uint32_t* ms)
{
    *ms = os_tick2ms(OS_GetSysTick());
    return 0;
}

int32_t colinkNetworkState(int32_t* state)
{
    uint8_t dhcpen;
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;

    if(get_if_config(&dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver))
    {
        *state = 0;
        return -1;
    }
    else if (ipaddr.u16[0] == 0 && ipaddr.u16[1] == 0)
    {
        *state = 0;
    }
    else
    {
        *state = 1;
    }

    return 0;
}

uint32_t colinkRand(void)
{
    return OS_Random();
}

uint32_t colinkStrlen(const char* src)
{
    return strlen(src);
}

char* colinkStrcpy(char* dst, const char* src)
{
    return strcpy(dst, src);
}

char* colinkStrncpy(char* dst, const char* src, uint32_t len)
{
    return strncpy(dst, src, len);
}

char* colinkStrcat(char* dst, const char* src)
{
    return strcat(dst, src);
}

char* colinkStrncat(char* dst, const char* src, uint32_t len)
{
    return strncat(dst, src, len);
}

char *colinkStrstr(const char *s1, const char *s2)
{
    return strstr(s1,s2);
}

int32_t colinkStrcmp(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}

int32_t colinkStrncmp(const char* str1, const char* str2, uint32_t len)
{
    return strncmp(str1, str2, len);
}

char* colinkStrchr(char* str, int32_t ch)
{
    return strchr(str, ch);
}

char* colinkStrrchr(const char* str, char c)
{
    return strrchr(str, c);
}

int32_t colinkAtoi(const char* str)
{
    return atoi(str);
}

int32_t colinkSprintf(char* buf, const char* format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsprintf(buf, format, args);
    va_end(args);

    return ret;
}

int32_t colinkSnprintf(char* buf, uint32_t size, const char* format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsnprintf(buf, size, format, args);
    va_end(args);

    return ret;
}

static void out_timestamp(void)
{
	extern uint32_t mytime_get_time(void *ptime);
	int32_t i, j, k;
	char timestr[16];

	k = mytime_get_time(NULL);
	k += 8 * 3600; //to beijing time
	k = k % (24 * 3600); //sec in day
	i = k / 3600; //hour
	k = k % 3600;
	j = k / 60; //min
	k = k % 60; //sec
	sprintf(timestr, "[%02d:%02d:%02d.%03d]", (int)i, (int)j, (int)k, (int)(OS_GetSysTick()%1000));
	drv_uart_write_fifo((unsigned char*)timestr, 14, UART_BLOCKING);
}

int32_t colinkPrintf(const char* format, ...)
{
    va_list ap;
    int32_t i, j, ret;

    va_start(ap, format);
    //ret = vprintf(format, ap);
    ret = vsnprintf(printf_buffer, PRINTF_BUFFER_SIZE, format, ap);
    va_end(ap);
    
	if(ret > PRINTF_BUFFER_SIZE) return 0;

	j = 0;
	for (i=0; i<ret; ++i)
	{
		if(printf_buffer[i] == '\n')
		{
			if(i>j) drv_uart_write_fifo((unsigned char*)printf_buffer+j, i-j, UART_BLOCKING);
			j = i+1;
			drv_uart_write_fifo((const uint8_t*)"\r\n", 2, UART_BLOCKING);
			out_timestamp();
		}
	}
	if(j < ret)
		drv_uart_write_fifo((unsigned char*)printf_buffer+j, ret-j, UART_BLOCKING);

	return ret;
}

void* colinkMemset(void* dst, int32_t c, uint32_t len)
{
    return memset(dst, c, len);
}

void* colinkMemcpy(void* dst, const void* src, uint32_t len)
{
    return memcpy(dst, src, len);
}

int32_t colinkMemcmp(const void* buf1, const void* buf2, uint32_t len)
{
    return memcmp(buf1, buf2, len);
}

void* colinkMalloc(uint32_t n)
{
    return OS_MemAlloc(n);
}

void *colinkRealloc(void *ptr, uint32_t n)
{
    return OS_MemRealloc(ptr, n);
}

void colinkFree(void* pt)
{
    OS_MemFree(pt);
}

uint16_t colinkHtons(uint16_t hs)
{
    return htons(hs);
}

uint16_t colinkNtohs(uint16_t ns)
{
    return ntohs(ns);
}

int32_t colinkSscanf(const char *s, const char *format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = sscanf(s, format, args);
    va_end(args);

    return ret;
}

double colinkStrtod(const char *nptr,char **endptr)
{
    return strtod(nptr, endptr);
}

int32_t colinkTolower(int32_t c)
{
    return tolower(c);
}

