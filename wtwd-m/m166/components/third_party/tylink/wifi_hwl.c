/***********************************************************
*  File: wifi_hwl.c
*  Author: nzy
*  Date: 20170914
***********************************************************/
#define _WIFI_HWL_GLOBAL
#include <stdio.h>
#include <string.h>
#include "wifi_hwl.h"
#include "ieee80211_mgmt.h"
#include "wificonf.h"
#include "wifi_api.h"
#include "softap_func.h"
#include "wifi_pkg_cap/smart_config.h"
#include "tuya_uni_semaphore.h"
#include "tuya_uni_thread.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define SCAN_MAX_AP 32
typedef struct {
    AP_IF_S *ap_if;
    BYTE_T ap_if_nums;
    BYTE_T ap_if_count;
    SEM_HANDLE sem_handle;
}SACN_AP_RESULT_S;

#define NETIF_STA_IDX 0 // lwip station/lwip ap interface  station/soft ap mode
#define NETIF_AP_IDX 1 // lwip ap interface
//extern struct netif xnetif[NET_IF_NUM];

#define UNVALID_SIGNAL -127
/***********************************************************
*************************variable define********************
***********************************************************/
STATIC WF_WK_MD_E wf_mode = WWM_LOWPOWER;
STATIC SNIFFER_CALLBACK snif_cb = NULL;
STATIC THRD_HANDLE dhcp_thrd = NULL;

static SEM_HANDLE scan_sem;
static UINT_T *wifi_ap_num;
static AP_IF_S *wifi_ap_ary;

/***********************************************************
*************************function define********************
***********************************************************/
//STATIC VOID __dhcp_thread(PVOID pArg);

STATIC void __hwl_wifi_scan_cb(void *data)
{
	AP_IF_S *ap_ary=wifi_ap_ary;

	*wifi_ap_num = (UINT_T)getAvailableIndex();
    for(int i=0; i<*wifi_ap_num; i++)
    {
	    ap_ary->channel = ap_list[i].channel;
    	ap_ary->rssi = ap_list[i].rssi;
    	memcpy(ap_ary->bssid, ap_list[i].mac, 6);
		memcpy(ap_ary->ssid, ap_list[i].name, WIFI_SSID_LEN+1);
    	ap_ary->s_len = ap_list[i].name_len;
        printf("%2d - name:%32s, rssi:-%2d CH:%2d mac:%02x-%02x-%02x-%02x-%02x-%02x\n", i, 
			ap_ary->ssid, ap_ary->rssi, ap_ary->channel, 
			ap_ary->bssid[0], ap_ary->bssid[1], ap_ary->bssid[2], ap_ary->bssid[3], ap_ary->bssid[4], ap_ary->bssid[5]);
		ap_ary++;
    }
	tuya_PostSemaphore(scan_sem);
}

/***********************************************************
*  Function: hwl_wf_all_ap_scan
*  Input: none
*  Output: ap_ary num
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_all_ap_scan(OUT AP_IF_S **ap_ary, OUT UINT_T *num)
{
    if(NULL == ap_ary || NULL == num) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET op_ret = OPRT_OK;
    wifi_ap_ary = OS_MemAlloc(SIZEOF(AP_IF_S) * SCAN_MAX_AP);
    if(NULL == wifi_ap_ary) {
        op_ret = OPRT_MALLOC_FAILED;
        goto ERR_RET;
    }
    memset(wifi_ap_ary, 0, SIZEOF(AP_IF_S) * SCAN_MAX_AP);

    op_ret = tuya_CreateAndInitSemaphore(&scan_sem, 0, 1);
    if(OPRT_OK != op_ret) {
        goto ERR_RET;
    }

    if(scan_AP(__hwl_wifi_scan_cb)) {
        op_ret = OPRT_WF_AP_SACN_FAIL;
        goto ERR_RET;
    }
    tuya_WaitSemaphore(scan_sem);

	wifi_ap_num = num;
	*ap_ary = wifi_ap_ary;

    tuya_ReleaseSemaphore(scan_sem);

    return OPRT_OK;

ERR_RET:
    tuya_ReleaseSemaphore(scan_sem);
    OS_MemFree(wifi_ap_ary);

    return op_ret;
}

STATIC void __hwl_wifi_assign_scan_cb(void *data)
{
	*wifi_ap_num = (UINT_T)getAvailableIndex();
    for(int i=0; i<*wifi_ap_num; i++)
    {
		if(memcmp(wifi_ap_ary->ssid, ap_list[i].name, wifi_ap_ary->s_len+1) == 0)
		{
	    	wifi_ap_ary->channel = ap_list[i].channel;
	    	wifi_ap_ary->rssi = ap_list[i].rssi;
    		memcpy(wifi_ap_ary->bssid, ap_list[i].mac, 6);
			*wifi_ap_num = 1;
			break;
		}
    }
	tuya_PostSemaphore(scan_sem);
}

/***********************************************************
*  Function: hwl_wf_assign_ap_scan
*  Input: ssid
*  Output: ap
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_assign_ap_scan(IN CONST CHAR_T *ssid, OUT AP_IF_S **ap)
{
    if(NULL == ssid || NULL == ap) {
        return OPRT_INVALID_PARM;
    }
	int ssidlen = strlen(ssid);

    OPERATE_RET op_ret = OPRT_OK;
    wifi_ap_ary = OS_MemAlloc(SIZEOF(AP_IF_S));
    if(NULL == wifi_ap_ary) {
        op_ret = OPRT_MALLOC_FAILED;
        goto ERR_RET;
    }
    memset(wifi_ap_ary, 0, SIZEOF(AP_IF_S));
    memcpy(wifi_ap_ary->ssid, ssid, ssidlen+1);
    wifi_ap_ary->s_len = ssidlen;
    
    op_ret = tuya_CreateAndInitSemaphore(&scan_sem, 0, 1);
    if(OPRT_OK != op_ret) {
        goto ERR_RET;
    }

	*wifi_ap_num = 0;
    if(scan_AP(__hwl_wifi_scan_cb)) {
        op_ret = OPRT_WF_AP_SACN_FAIL;
        goto ERR_RET;
    }
    tuya_WaitSemaphore(scan_sem);

	PR_DEBUG("scan count:%d", *wifi_ap_num);
    if(0 == *wifi_ap_num) {
        op_ret = OPRT_WF_NOT_FIND_ASS_AP;
        goto ERR_RET;
    }
    *ap = wifi_ap_ary;

    tuya_ReleaseSemaphore(scan_sem);
    return OPRT_OK;

ERR_RET:
    tuya_ReleaseSemaphore(scan_sem);
    OS_MemFree(wifi_ap_ary);

    return op_ret;
}

/***********************************************************
*  Function: hwl_wf_release_ap
*  Input: ap
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_release_ap(IN AP_IF_S *ap)
{
    if(NULL == ap) {
        return OPRT_INVALID_PARM;
    }

    OS_MemFree(ap);
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_set_cur_channel
*  Input: chan
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_set_cur_channel(IN CONST BYTE_T chan)
{
    set_channel(chan);
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_get_cur_channel
*  Input: none
*  Output: chan
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_get_cur_channel(OUT BYTE_T *chan)
{
    *chan = get_current_channel();
    return OPRT_OK;
}

STATIC VOID __hwl_promisc_callback(packetinfo *packet)
{
    if(NULL == snif_cb) {
        return;
    }

    snif_cb((BYTE_T*)packet->data, (USHORT_T)packet->len);
}

STATIC VOID ty_ch_change_cb(IN BYTE_T ch)
{
    set_channel(ch);
}

/***********************************************************
*  Function: hwl_wf_sniffer_set
*  Input: en cb
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_sniffer_set(IN CONST BOOL_T en,IN CONST SNIFFER_CALLBACK cb)
{
    if(TRUE == en && NULL == cb) {
        return OPRT_INVALID_PARM;
    }
	printf("%s en=%d\n", __func__, en);

    int ret = 0;
    if(TRUE == en) {
		snif_cb = cb;
		attach_sniffer_cb(RECV_DATA_BCN, &__hwl_promisc_callback, 512);
		attach_channel_change_cb(&ty_ch_change_cb);
		start_sniffer_mode();
		auto_ch_switch_start(100);
    }else {
		auto_ch_switch_stop();
		deattach_channel_change_cb();
		deattach_sniffer_cb();
		stop_sniffer_mode();
		snif_cb = NULL;
    }

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_get_ip
*  Input: wf
*  Output: ip
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_get_ip(IN CONST WF_IF_E wf,OUT NW_IP_S *ip)
{
    uint8_t dhcpen;
    uip_ipaddr_t ipaddr, submask, gateway, dnsserver;

	if(get_if_config(&dhcpen, &ipaddr, &submask, &gateway, &dnsserver) != 0)
		return OPRT_COM_ERROR;
    // ip
     sprintf(ip->ip,"%d.%d.%d.%d",ipaddr.u8[0],ipaddr.u8[1],ipaddr.u8[2],ipaddr.u8[3]);
    // gw
    sprintf(ip->gw,"%d.%d.%d.%d",gateway.u8[0],gateway.u8[1],gateway.u8[2],gateway.u8[3]);
    // submask
    sprintf(ip->mask,"%d.%d.%d.%d",submask.u8[0],submask.u8[1],submask.u8[2],submask.u8[3]);

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_set_ip
*  Input: wf
*  Output: ip
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_set_ip(IN CONST WF_IF_E wf,IN CONST NW_IP_S *ip)
{
    return OPRT_NOT_SUPPORTED;
}

/***********************************************************
*  Function: hwl_wf_get_mac
*  Input: wf
*  Output: mac
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_get_mac(IN CONST WF_IF_E wf,OUT NW_MAC_S *mac)
{
	if(mac == NULL) {
        return OPRT_INVALID_PARM;
	}

	void *cfg_handle = wifi_cfg_init();
	wifi_cfg_get_addr1(cfg_handle, mac->mac);
	wifi_cfg_deinit(cfg_handle);

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_set_mac
*  Input: wf mac
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_set_mac(IN CONST WF_IF_E wf,IN CONST NW_MAC_S *mac)
{
    return OPRT_NOT_SUPPORTED;
}

/***********************************************************
*  Function: hwl_wf_wk_mode_set
*  Input: mode
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_wk_mode_set(IN CONST WF_WK_MD_E mode)
{
	WIFI_OPMODE mode1;

	mode1 = get_DUT_wifi_mode();

	if(mode == WWM_STATION) mode1 = DUT_STA;
    if(mode == WWM_SOFTAP) mode1 = DUT_AP;
    if(mode == WWM_LOWPOWER) mode1 = DUT_CONCURRENT;
    if(mode == WWM_STATIONAP) mode1 = DUT_TWOSTA;
    if(mode == WWM_SNIFFER) mode1 = DUT_SNIFFER;

    DUT_wifi_start(mode1);

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_wk_mode_get
*  Input: none
*  Output: mode
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_wk_mode_get(OUT WF_WK_MD_E *mode)
{
	*mode = WWM_LOWPOWER;

	WIFI_OPMODE mode1 = get_DUT_wifi_mode();

	if(mode1 == DUT_STA) *mode = WWM_STATION;
    if(mode1 == DUT_AP) *mode = WWM_SOFTAP;
    if(mode1 == DUT_CONCURRENT) *mode = WWM_LOWPOWER;
    if(mode1 == DUT_TWOSTA) *mode = WWM_STATIONAP;
    if(mode1 == DUT_SNIFFER) *mode = WWM_SNIFFER;

    return OPRT_OK;
}
#if 0
static int find_ap_from_scan_buf(char*buf, int buflen, char *target_ssid, void *user_data)
{
    rtw_wifi_setting_t *pwifi = (rtw_wifi_setting_t *)user_data;
    int plen = 0;

    while(plen < buflen){
    	u8 len, ssid_len, security_mode;
    	char *ssid;

    	// len offset = 0
    	len = (int)*(buf + plen);
    	// check end
    	if(len == 0) break;
    	// ssid offset = 14
    	ssid_len = len - 14;
    	ssid = buf + plen + 14 ;
    	if((ssid_len == strlen(target_ssid))
    		&& (!memcmp(ssid, target_ssid, ssid_len)))
    	{
    		strcpy((char*)pwifi->ssid, target_ssid);
    		// channel offset = 13
    		pwifi->channel = *(buf + plen + 13);
    		// security_mode offset = 11
    		security_mode = (u8)*(buf + plen + 11);
    		if(security_mode == IW_ENCODE_ALG_NONE)
    			pwifi->security_type = RTW_SECURITY_OPEN;
    		else if(security_mode == IW_ENCODE_ALG_WEP)
    			pwifi->security_type = RTW_SECURITY_WEP_PSK;
    		else if(security_mode == IW_ENCODE_ALG_CCMP)
    			pwifi->security_type = RTW_SECURITY_WPA2_AES_PSK;
    		break;
    	}
    	plen += len;
    }
    return 0;
}

static int get_ap_security_mode(IN char * ssid, OUT rtw_security_t *security_mode)
{
	rtw_wifi_setting_t wifi;
	u32 scan_buflen = 1000;

	memset(&wifi, 0, sizeof(wifi));

	if(wifi_scan_networks_with_ssid(find_ap_from_scan_buf, (void*)&wifi, scan_buflen, ssid, strlen(ssid)) != RTW_SUCCESS){
		printf("Wifi scan failed!\n");
		return 0;
	}

	if(strcmp(wifi.ssid, ssid) == 0){
		*security_mode = wifi.security_type;
		return 1;
	}
		
	return 0;
}
#endif

/***********************************************************
*  Function: hwl_wf_station_connect
*  Input: ssid passwd
*  Output: mode
*  Return: OPERATE_RET
***********************************************************/
// only support wap/wap2 
extern void atwificbfunc(WIFI_RSP *msg);
OPERATE_RET hwl_wf_station_connect(IN CONST CHAR_T *ssid,IN CONST CHAR_T *passwd)
{
    int ret = 0;

    ret = set_wifi_config((u8*)ssid, strlen(ssid), (u8*)passwd, strlen(passwd), NULL, 0);
    if(0 != ret) {
        PR_ERR("ERROR: set_wifi_config:%d",ret);
        return OPRT_COM_ERROR;
    }

    ret = wifi_connect(atwificbfunc);
    if(0 != ret) {
        PR_ERR("ERROR: wifi_connect:%d",ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/*STATIC VOID __dhcp_thread(PVOID pArg)
{
    uint8_t ret = 0;
    int remain = 5;

    while(remain) {
        ret = LwIP_DHCP(NETIF_STA_IDX, DHCP_START);
        if(ret == DHCP_ADDRESS_ASSIGNED) {
            break;
        }
        remain--;
    }

    if(0 == remain) {
        PR_ERR("dhcp error");
    }
    PR_DEBUG("__dhcp_thread delete********");
    DeleteThrdHandle(dhcp_thrd);
    dhcp_thrd = NULL;
}*/

/***********************************************************
*  Function: hwl_wf_station_disconnect
*  Input: none
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_station_disconnect(VOID)
{
    wifi_disconnect(atwificbfunc);
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_station_get_conn_ap_rssi
*  Input: none
*  Output: rssi
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_station_get_conn_ap_rssi(OUT SCHAR_T *rssi)
{
    int ret = 0;
    int tmp_rssi = 0;
    
    ret = wifi_get_rssi(&tmp_rssi);
    if(ret < 0) {
        return OPRT_COM_ERROR;
    }

    *rssi = tmp_rssi;

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_station_stat_get
*  Input: none
*  Output: stat
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_station_stat_get(OUT WF_STATION_STAT_E *stat)
{
	if(get_wifi_status()) *stat = WSS_CONN_SUCCESS;
	else *stat = WSS_IDLE;

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_ap_start
*  Input: cfg
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_ap_start(IN CONST WF_AP_CFG_IF_S *cfg)
{
    if(NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

	SOFTAP_CUSTOM_CONFIG isoftap_config;

    //cfg->ssid_hidden;
	//isoftap_custom_config.start_ip = ;
	//isoftap_custom_config.end_ip = ;
	//isoftap_custom_config.gw = ;
	//isoftap_custom_config.subnet = ;
	isoftap_config.max_sta_num = cfg->max_conn;
	isoftap_config.encryt_mode = cfg->md;	
	isoftap_config.keylen = cfg->p_len;
	memcpy(isoftap_config.key, cfg->passwd, sizeof(isoftap_config.key));
	isoftap_config.channel = cfg->chan;
	isoftap_config.beacon_interval = cfg->ms_interval;

	isoftap_config.ssid_length = cfg->s_len;
	memcpy(isoftap_config.ssid, cfg->ssid, sizeof(isoftap_config.ssid));

	int32_t rlt = softap_set_custom_conf(&isoftap_config);
	if(rlt == -4) printf("Don't configure SoftAP when SoftAP is running. Please execute AT+AP_EXIT first\n");

	hwl_wf_ap_stop();

    DUT_wifi_start(DUT_AP);
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_ap_stop
*  Input: none
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET hwl_wf_ap_stop(VOID)
{
    softap_exit();

    WIFI_OPMODE mode = get_DUT_wifi_mode();
    
    if( mode != DUT_STA )
        DUT_wifi_start(DUT_STA);

    return OPRT_OK;
}

