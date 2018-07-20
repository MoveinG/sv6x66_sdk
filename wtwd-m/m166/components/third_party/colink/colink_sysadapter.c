#include "colink_define.h"
#include "colink_sysadapter.h"
#include "osal.h"
#include <string.h>
#include <stdarg.h>

#define PRINTF_BUFFER_SIZE (512)
static char printf_buffer[PRINTF_BUFFER_SIZE];

int colinkGettime(uint32_t* ms)
{
    *ms = os_tick2ms(OS_GetSysTick());
    return 0;
}

unsigned long colinkRand(void)
{
    return OS_Random();
}

unsigned int colinkStrlen(const char* src)
{
    return strlen(src);
}

char* colinkStrcpy(char* dst, const char* src)
{
    return strcpy(dst, src);
}

char* colinkStrncpy(char* dst, const char* src, unsigned int len)
{
    return strncpy(dst, src, len);
}

char* colinkStrcat(char* dst, const char* src)
{
    return strcat(dst, src);
}

char* colinkStrncat(char* dst, const char* src, unsigned int len)
{
    return strncat(dst, src, len);
}

char *colinkStrstr(const char *s1, const char *s2)
{
    return strstr(s1,s2);
}

int colinkStrcmp(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}

int colinkStrncmp(const char* str1, const char* str2, unsigned int len)
{
    return strncmp(str1, str2, len);
}

char* colinkStrchr(char* str, int ch)
{
    return strchr(str, ch);
}

char* colinkStrrchr(const char* str, char c)
{
    return strrchr(str, c);
}

int colinkAtoi(const char* str)
{
    return atoi(str);
}

int colinkSprintf(char* buf, const char* format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vsprintf(buf, format, args);
    va_end(args);

    return ret;
}

int colinkSnprintf(char* buf, unsigned int size, const char* format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vsnprintf(buf, size, format, args);
    va_end(args);

    return ret;
}

int colinkPrintf(const char* format, ...)
{
    va_list ap;
    int ret;

    va_start(ap, format);
    //ret = vprintf(format, ap);
    ret = vsnprintf(printf_buffer, PRINTF_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    os_printf("%s", printf_buffer);
    return ret;
}

void* colinkMemset(void* dst, int c, unsigned int len)
{
    return memset(dst, c, len);
}

void* colinkMemcpy(void* dst, const void* src, unsigned int len)
{
    return memcpy(dst, src, len);
}

int colinkMemcmp(const void* buf1, const void* buf2, unsigned int len)
{
    return memcmp(buf1, buf2, len);
}

void* colinkMalloc(unsigned long n)
{
    return OS_MemAlloc(n);
}

void colinkFree(void* pt)
{
    OS_MemFree(pt);
}

int colinkNetworkState(int* state)
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

