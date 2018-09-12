#include "colink_define.h"
#include "colink_sysadapter.h"
#include "osal.h"
#include <string.h>
#include <stdarg.h>

#define PRINTF_BUFFER_SIZE (512)
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

int32_t colinkPrintf(const char* format, ...)
{
    va_list ap;
    int32_t ret;

    va_start(ap, format);
    //ret = vprintf(format, ap);
    ret = vsnprintf(printf_buffer, PRINTF_BUFFER_SIZE, format, ap);
    va_end(ap);
    
	/*uint8_t ch; 
	int i = strlen((char*)printf_buffer);

	ch = printf_buffer[120];
	printf_buffer[120] = 0;
	os_printf("%s", printf_buffer);
	printf_buffer[120] = ch;

	if(i > 240)
	{
		ch = printf_buffer[240];
		printf_buffer[240] = 0;
	    os_printf("%s", printf_buffer+120);
		printf_buffer[240] = ch;

	    os_printf("%s", printf_buffer+240);
	}
	else if(i > 120)
	{
	    os_printf("%s", printf_buffer+120);
	}*/

    os_printf("%s", printf_buffer);
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

