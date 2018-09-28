#include <stdint.h>
#include <flash.h>
#include "sa_cfg.h"
#include "osal.h"
#include <string.h>
#include <stdio.h>

#define CFG_ASSERT(cmp) \
    do { \
    } while(!(cmp))

extern const struct sa_cfg g_sa_cfg __attribute__((section(".sa_mp_data")));
const struct sa_cfg g_sa_cfg __attribute__((section(".sa_mp_data"))) = {
    .buf_mp = {
		10,10,8,9,9,9,8,158,158,7,9,10,7,7,7,7,7,1, //RT Config
		11,11,9,10,10,10,9,195,195,7,9,10,7,7,7,7,7,1, //HT Config
		9,9,7,8,8,8,7,170,170,7,9,10,9,7,7,8,7,0, //LT Config
		4,4, //RF Gain, B Rate Gain
		13,11,9,7, //G Rate Gain
		13,11,9,7, //20N Rate Gain
		9,7,6,5, //40N Rate Gain
		30,90, //Temperature Boundary
		0x00,0x00,
		0x0D,0x0D,0x0A,0x08,0x4A,0x92,0x4D,0x92,0xCC,0xB6,0xCA,0xB6,//5G RT Config
		0x0D,0x0D,0x0A,0x08,0x4A,0x92,0x4D,0x92,0xCC,0xB6,0xCA,0xB6,//5G HT Config
		0x0D,0x0D,0x0A,0x08,0x4A,0x92,0x4D,0x92,0xCC,0xB6,0xCA,0xB6,//5G LT Config
		0x50,0x14,0x7C,0x15,0x44,0x16 //5G Band Threshold
	}
};

void cfg_dump_sa_pre_table() {
    int i;
    volatile uint8_t *ptr = (volatile uint8_t *)&g_sa_cfg;
    ptr -= 256;
    for (i = 0; i < 256; i++) {
        if ((i!=0) && (i%36 == 0)) {
            printf("\n");
        }
        printf("%02X-", ptr[i]);
    }
    printf("\n");
}
void cfg_dump_sa_post_table() {
    extern uint8_t __lds_sa_mp_data_end;
    int i;
    volatile uint8_t *ptr = (volatile uint8_t *)&__lds_sa_mp_data_end;
    for (i = 0; i < 256; i++) {
        if ((i!=0) && (i%36 == 0)) {
            printf("\n");
        }
        printf("%02X-", ptr[i]);
    }
    printf("\n");
}
void cfg_dump_sa_table(int len) {
    int i;
    volatile uint8_t *ptr = (volatile uint8_t *)&g_sa_cfg;
    for (i = 0; i < len; i++) {
        if ((i!=0) && (i%36 == 0)) {
            printf("\n");
        }
        printf("%02X-", ptr[i]);
    }
    printf("\n");
}

void cfg_sa_write_cfg(struct sa_cfg *new_cfg, uint16_t len) {
    uint8_t *buf;
    CFG_ASSERT(len <= FLASH_SECTOR_SIZE);
    flash_init();
    if(len < FLASH_SECTOR_SIZE)
    {
        buf = OS_MemAlloc(FLASH_SECTOR_SIZE);
        memcpy(&(buf[0]), (uint8_t*)new_cfg, len);
        memcpy(&(buf[len]), (((uint8_t *)&g_sa_cfg)+len), FLASH_SECTOR_SIZE-len);
    }
    else buf = (uint8_t*)new_cfg;

    uint32_t ptr = (uint32_t)((uint32_t)(&g_sa_cfg) & (0xFFFFFF));
    OS_DeclareCritical();
    OS_EnterCritical();
    flash_sector_erase((unsigned int)ptr);
    int i;
    
    for (i = 0; i < FLASH_SECTOR_SIZE; i+=FLASH_PAGE_SIZE) {
        flash_page_program(ptr+i, FLASH_PAGE_SIZE, &(buf[i]));
    }
    OS_ExitCritical();

    if(len < FLASH_SECTOR_SIZE) OS_MemFree(buf);
}

