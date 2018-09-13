/***********************************************************
*  File: tuya_main.c
*  Author: nzy
*  Date: 20171012
***********************************************************/
#include "adapter_platform.h"
#include "tuya_iot_wifi_api.h"
//#include "flash_api.h"
//#include <device_lock.h>
#include "tuya_device.h"
#include "mf_test.h"
#include "tuya_uart.h"
#include "tuya_gpio.h"
#include "gw_intf.h"
#include "wf_basic_intf.h"
#include "fsal.h"
#include "wdt/drv_wdt.h"
#include "ota_api.h"

#define PVOID	PVOID_T
#define UINT	UINT_T

/***********************************************************
*************************micro define***********************
***********************************************************/
#define TEST_SSID "tuya_mdev_test1"
#define OTA_BIN_FILENAME	"ota.bin"
#define OTA_MD5_FILENAME	"ota_info.bin"
extern spiffs* fs_handle;
/***********************************************************
*************************variable define********************
***********************************************************/
#if 0
typedef enum {
    UGS_RECV_HEADER = 0,
    UGS_RECV_OTA_HD,
    UGS_SEARCH_SIGN,
    UGS_RECV_SIGN,
    UGS_RECV_IMG_DATA,
    // UGS_RECV_RDP_DATA,
    UGS_FINISH
}UG_STAT_E;

typedef struct {
    UG_STAT_E stat;
    update_ota_target_hdr hdr;
    u32 ota_index;

    u32 image_cnt;
    u8 signature[8];
    update_dw_info DownloadInfo[2];
    u32 cur_image_cnt[2];

    flash_t flash;
    u32 new_addr;
    u32 recv_data_cnt;
}UG_PROC_S;

STATIC UG_PROC_S *ug_proc = NULL;
#endif

extern VOID app_init(VOID);
typedef VOID (*APP_PROD_CB)(BOOL_T flag, CHAR_T rssi);//lql
STATIC APP_PROD_CB app_prod_test = NULL;//lql
STATIC GW_WF_CFG_MTHD_SEL gwcm_mode = GWCM_OLD;//lql

/***********************************************************
*************************function define********************
***********************************************************/
extern VOID pre_device_init(VOID);
extern OPERATE_RET device_init(VOID);
extern BOOL_T gpio_test(VOID);
extern void sys_jtag_off(void);
extern TY_GPIO_PORT_E swith_ctl_port;
STATIC VOID __gw_ug_inform_cb(IN CONST FW_UG_S *fw);
STATIC OPERATE_RET __gw_upgrage_process_cb(IN CONST FW_UG_S *fw, IN CONST UINT total_len,IN CONST UINT offset,\
                                                      IN CONST BYTE_T *data,IN CONST UINT len,OUT UINT *remain_len, IN PVOID pri_data);
STATIC VOID __gw_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID pri_data);

STATIC OPERATE_RET __mf_gw_upgrade_notify_cb(VOID);
STATIC VOID __mf_gw_ug_inform_cb(VOID);

STATIC VOID __tuya_mf_uart_init(UINT_T baud,UINT_T bufsz)
{
    ty_uart_init(TY_UART0,baud,TYWL_8B,TYP_NONE,TYS_STOPBIT1,bufsz);
}
STATIC VOID __tuya_mf_uart_free(VOID)
{
    ty_uart_free(TY_UART0);
}

STATIC VOID __tuya_mf_send(IN CONST BYTE_T *data,IN CONST UINT len)
{
    ty_uart_send_data(TY_UART0,data,len);
}

STATIC UINT __tuya_mf_recv(OUT BYTE_T *buf,IN CONST UINT len)
{
    return ty_uart_read_data(TY_UART0,buf,len);
}

STATIC BOOL_T scan_test_ssid(VOID)
{
	//if(FALSE == get_new_prod_mode()) {
		//return false;
	//}

	OPERATE_RET op_ret = OPRT_OK;
	//special for GWCM_OLD_PROD.......only do prodtesting when in smartlink or ap mode
	if(gwcm_mode == GWCM_OLD_PROD ) {
        op_ret = wd_gw_wsm_read(&(get_gw_cntl()->gw_wsm));
        if(get_gw_cntl()->gw_wsm.nc_tp >= GWNS_TY_SMARTCFG) {
            return false;
        }
	}

	wf_wk_mode_set(WWM_STATION);
	AP_IF_S *ap = NULL;
	BOOL_T flag = TRUE;
	op_ret = wf_assign_ap_scan(TEST_SSID, &ap);//lql
	wf_station_disconnect();
	if(OPRT_OK != op_ret) {
	    PR_NOTICE("wf_assign_ap_scan failed(%d)",op_ret);
		return FALSE;
	}
    //check if has authorized
    op_ret = wd_gw_base_if_read(&(get_gw_cntl()->gw_base));
    if(OPRT_OK != op_ret) {
        PR_DEBUG("read flash err");
        flag = FALSE;
    }
    // gateway base info verify
    if(0 == get_gw_cntl()->gw_base.auth_key[0] || \
       0 == get_gw_cntl()->gw_base.uuid[0]) {
        PR_DEBUG("please write uuid and auth_key first");
        flag = FALSE;
    }
	
	if(app_prod_test) {
		app_prod_test(flag, ap->rssi);
	}
	return TRUE;
}

void app_cfg_set(IN CONST GW_WF_CFG_MTHD_SEL mthd, APP_PROD_CB callback)
{
	app_prod_test = callback;
	gwcm_mode = mthd;
}

/***********************************************************
*  Function: user_main 
*  Input: none
*  Output: none
*  Return: none
***********************************************************/
void user_main(void)
{
    OPERATE_RET op_ret = OPRT_OK;
    op_ret = tuya_iot_init(NULL);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_init err:%d",op_ret);
        return;
    }

    pre_device_init();
#if 1
    // to add prodect test code
    mf_reg_gw_ug_cb(__mf_gw_ug_inform_cb, __gw_upgrage_process_cb, __mf_gw_upgrade_notify_cb);
    MF_IMPORT_INTF_S intf = {
        __tuya_mf_uart_init,
        __tuya_mf_uart_free,
        __tuya_mf_send,
        __tuya_mf_recv,
        gpio_test,
    };
    op_ret = mf_init(&intf,APP_BIN_NAME,USER_SW_VER,TRUE); //TRUE: write MAC address
    if(OPRT_OK != op_ret) {
        PR_ERR("mf_init err:%d",op_ret);
        return;
    }
    ty_uart_free(TY_UART0);
    PR_NOTICE("mf_init succ"); 
#endif
    OS_MsDelay(2000);
    // register gw upgrade inform callback
    tuya_iot_force_reg_gw_ug_cb(__gw_ug_inform_cb);

// for debug
#if 0
    WF_GW_PROD_INFO_S wf_prod_info = {
        "003tuyatestf7f149189","NeA8Wc7srpAZHEMuru867oblOLN2QCC5",NULL,NULL
    };
    op_ret = tuya_iot_set_wf_gw_prod_info(&wf_prod_info);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_set_wf_gw_prod_info err:%d",op_ret);
        return;
    }
#endif
    app_init();
    PR_DEBUG("gwcm_mode %d",gwcm_mode);
    if(gwcm_mode != GWCM_OLD) {
        PR_DEBUG("low_power select");
    	if(true == scan_test_ssid()) {
    		PR_DEBUG("prodtest");		
    		return;
    	}
        PR_DEBUG("no tuya_mdev_test1!");
        op_ret = device_init();
        if(OPRT_OK != op_ret) {
            PR_ERR("device_init error:%d",op_ret);
            return;
        }
    }else {
        PR_DEBUG("device_init in");
        op_ret = device_init();
        if(OPRT_OK != op_ret) {
            PR_ERR("device_init err:%d",op_ret);
            return;
        }
    }
}
// mf gateway upgrade start 
VOID __mf_gw_ug_inform_cb(VOID)
{
	#if 0
    ug_proc = Malloc(SIZEOF(UG_PROC_S));
    if(NULL == ug_proc) {
        PR_ERR("malloc err");
        return;
    }
    memset(ug_proc,0,SIZEOF(UG_PROC_S));
    
    if (ota_get_cur_index() == OTA_INDEX_1) {
        ug_proc->ota_index = OTA_INDEX_2;
        PR_DEBUG("OTA2 address space will be upgraded\n");
    } else {
        ug_proc->ota_index = OTA_INDEX_1;
        PR_DEBUG("OTA1 address space will be upgraded\n");
    }
  #endif
  
}

// gateway upgrade start 
STATIC VOID __gw_ug_inform_cb(IN CONST FW_UG_S *fw)
{
    /*ug_proc = Malloc(SIZEOF(UG_PROC_S));
    if(NULL == ug_proc) {
        PR_ERR("malloc err");
        return;
    }
    memset(ug_proc,0,SIZEOF(UG_PROC_S));*/
    OPERATE_RET op_ret = OPRT_OK;
    op_ret = tuya_iot_upgrade_gw(fw,__gw_upgrage_process_cb,__gw_upgrade_notify_cb,NULL);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_upgrade_gw err:%d",op_ret);
        return;
    }

    /*memset(ug_proc,0,SIZEOF(UG_PROC_S));
    if (ota_get_cur_index() == OTA_INDEX_1) {
        ug_proc->ota_index = OTA_INDEX_2;
        PR_DEBUG("OTA2 address space will be upgraded\n");
    } else {
        ug_proc->ota_index = OTA_INDEX_1;
        PR_DEBUG("OTA1 address space will be upgraded\n");
    }*/
}
// mf gateway upgrade result notify
OPERATE_RET __mf_gw_upgrade_notify_cb(VOID)
{
	#if 0
    // verify 
    u32 ret = 0;
    ret = verify_ota_checksum(ug_proc->new_addr,ug_proc->DownloadInfo[0].ImageLen, \ 
                              ug_proc->signature,&ug_proc->hdr);
    if(1 != ret) {
        PR_ERR("verify_ota_checksum err");

        device_mutex_lock(RT_DEV_LOCK_FLASH);
        flash_erase_sector(&ug_proc->flash, ug_proc->new_addr - SPI_FLASH_BASE);
        device_mutex_unlock(RT_DEV_LOCK_FLASH);
        return OPRT_COM_ERROR;
    }

    if(!change_ota_signature(ug_proc->new_addr, ug_proc->signature, ug_proc->ota_index)) {
        PR_ERR("change signature failed\n");
        return OPRT_COM_ERROR;
    }

    PR_NOTICE("the gateway upgrade success");
 #endif
 
 
    return OPRT_OK;
}

// gateway upgrade result notify
STATIC VOID __gw_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID pri_data)
{
	printf("url=%s\n", fw->fw_url);
	printf("%s tp=%d, ver=%s, size=%d, result=%d\n", __func__, fw->tp, fw->sw_ver, fw->file_size, download_result);
	printf("%s\n", fw->fw_md5);

    if(OPRT_OK == download_result) // update success
	{
		SSV_FILE fd = FS_open(fs_handle, OTA_MD5_FILENAME, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
		if(fd < 0) return;

		uint32_t write_len = FS_write(fs_handle, fd, fw->fw_md5, sizeof(fw->fw_md5));
		FS_close(fs_handle, fd);
		//if(write_len == sizeof(fw->fw_md5))
		{
			ota_set_parameter(OTA_BIN_FILENAME, "");
			ota_update();

			drv_wdt_init(); //reboot
			drv_wdt_enable(SYS_WDT, 100);
		}
	}
	else FS_remove(fs_handle, OTA_BIN_FILENAME);
}

// gateway upgrade process
STATIC OPERATE_RET __gw_upgrage_process_cb(IN CONST FW_UG_S *fw, IN CONST UINT total_len,IN CONST UINT offset,\
                                                      IN CONST BYTE_T *data,IN CONST UINT len,OUT UINT *remain_len, IN PVOID pri_data)
{
	*remain_len = 0;
	//printf("%s tp=%d, ver=%s, size=%d\n", __func__, fw->tp, fw->sw_ver, fw->file_size);
	//printf("total_len=%d, offset=%d, data=0x%x, len=%d, *remain_len=%d, pri_data=%x\n", total_len, offset, data, len, *remain_len, pri_data);

	SSV_FILE fd = FS_open(fs_handle, OTA_BIN_FILENAME, SPIFFS_CREAT | SPIFFS_RDWR, 0);
	if(fd < 0) return OPRT_WR_FLASH_ERROR;

	int32_t seek_pos = FS_lseek(fs_handle, fd, offset, SPIFFS_SEEK_SET);
	if(offset != seek_pos)
	{
		printf("seek_pos=%d, offset=%d\n", (int)seek_pos, offset);
		FS_close(fs_handle, fd);
		return OPRT_WR_FLASH_ERROR;
	}
	int32_t write_len = FS_write(fs_handle, fd, (void*)data, len);
	if(write_len != len)
	{
		printf("write_len=%d, len=%d\n", (int)write_len, len);
		FS_close(fs_handle, fd);
		return OPRT_WR_FLASH_ERROR;
	}
	FS_close(fs_handle, fd);

    return OPRT_OK;
}


