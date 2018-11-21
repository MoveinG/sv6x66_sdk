#include "sys/backtrace.h"
#include "sys/flash.h"
#if defined(XIP_MODE)
#include "sys/xip.h"
#endif
#include "fsal.h"
#include "osal.h"
#include "wifinetstack.h"
#include "idmanage/pbuf.h"
#include "security/drv_security.h"
#include "phy/drv_phy.h"
#include "soc_defs.h"
#include "ieee80211_mgmt.h"
#include "ieee80211_mac.h"
#include "sta_func.h"
#include "wifi_api.h"
#include "netstack_def.h"
#include "netstack.h"
#include "uart/drv_uart.h"
#include "rf/rf_api.h"

void Cli_Task( void *args );

/**********************************************************/
SSV_FS fs_handle = NULL;

void temperature_compensation_task(void *pdata)
{
    printf("%s\n", __func__);
    OS_MsDelay(1*1000);
    load_rf_table_from_flash();
    write_reg_rf_table();
    while(1)
    {
        OS_MsDelay(3*1000);
        do_temerature_compensation();
    }
    OS_TaskDelete(NULL);
}

void ssvradio_init_task(void *pdata)
{
    printf("%s\n", __func__);
    PBUF_Init();
    NETSTACK_RADIO.init();    
    drv_sec_init();
    netstack_init(NULL);
    load_phy_table();
    DUT_wifi_start(DUT_STA);    
    OS_TaskDelete(NULL);
}

void ada_init_task(void *pdata){
    ada_demo_main();
    OS_TaskDelete(NULL);
}

void ayla_connect_cb(WIFI_RSP *msg)
{
    printf("=========\n");
    printf("ayla_connect_cb wifistatus = %d\n", msg->wifistatus);
    
    uint8_t dhcpen;
    u8 mac[6];
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;

    if(msg->wifistatus == 1)
    {
        if(msg->id == 0)
            get_if_config_2("et0", &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
        else
            get_if_config_2("et1", &dhcpen, (u32*)&ipaddr, (u32*)&submask, (u32*)&gateway, (u32*)&dnsserver, mac, 6);
        printf("Wifi Connect\n");
        printf("STA%d:\n", msg->id);
        printf("mac addr        - %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        printf("ip addr         - %d.%d.%d.%d\n", ipaddr.u8[0], ipaddr.u8[1], ipaddr.u8[2], ipaddr.u8[3]);
        printf("netmask         - %d.%d.%d.%d\n", submask.u8[0], submask.u8[1], submask.u8[2], submask.u8[3]);
        printf("default gateway - %d.%d.%d.%d\n", gateway.u8[0], gateway.u8[1], gateway.u8[2], gateway.u8[3]);
        printf("DNS server      - %d.%d.%d.%d\n", dnsserver.u8[0], dnsserver.u8[1], dnsserver.u8[2], dnsserver.u8[3]);

        OS_TaskCreate(ada_init_task, "ada_init_task", 1024, NULL, OS_TASK_PRIO3, NULL);
    }
    else
    {
        printf("Wifi Disconnect  reson:%d\n",msg->reason);
    }
}

#define AP_SSID  "A15"
#define AP_KEY   "cdd@20180725"

void connect_task(void *pdata){
    OS_MsDelay(500);
    wifi_connect_active_2(AP_SSID, strlen(AP_SSID), AP_KEY, strlen(AP_KEY),ayla_connect_cb,5);
    OS_TaskDelete(NULL);
}

extern void drv_uart_init(void);
void APP_Init(void)
{
#ifdef XIP_MODE
	xip_init();
	xip_enter();
#endif
	drv_uart_init();
    drv_uart_set_fifo(UART_INT_RXFIFO_TRGLVL_1, 0x0);
	drv_uart_set_format(921600, UART_WORD_LEN_8, UART_STOP_BIT_1, UART_PARITY_DISABLE);

	OS_Init();
	OS_StatInit();
	OS_MemInit();
	OS_PsramInit();

	fs_handle = FS_init();
	if(fs_handle)
	{
		FS_remove_prevota(fs_handle);
	}
    FS_list(fs_handle);

    OS_TaskCreate(Cli_Task, "cli", 512, NULL, OS_TASK_PRIO3, NULL);

    OS_TaskCreate(ssvradio_init_task, "ssvradio_init", 256, NULL, OS_TASK_PRIO3, NULL);

    //this task run in low priority
    OS_TaskCreate(temperature_compensation_task, "rf temperature compensation", 256, NULL, OS_TASK_PRIO1, NULL);

    OS_TaskCreate(connect_task, "connect_task", 512, NULL, OS_TASK_PRIO3, NULL);
    printf("[%s][%d] string\n", __func__, __LINE__);

    OS_StartScheduler();
}

void vAssertCalled( const char *func, int line )
{
    printf("<!!!OS Assert!!!> func = %s, line=%d\n", func, line);
    print_callstack();
    while(1);
}

