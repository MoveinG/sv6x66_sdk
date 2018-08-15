#include "wificonf.h"
#include "wifinetstack.h"
#include "wifi_api.h"
#include "ieee80211.h"
#include "osal/osal.h"
#include "phy/drv_phy.h"
#include "wifi_pkg_cap/smart_config.h"
#include "wifi_api.h"
#include "esp_touch.h"
#include "lwip/sockets.h"

////////////////////////////
#define MAC_BROADCAST	"\xFF\xFF\xFF\xFF\xFF\xFF"
#define MAC_MULTICAST	"\x01\x00\x5E"

#define ESPTOUCH_GUIDE_CODE			515
#define ESPTOUCH_PACKET_SUB			40

#define GUIDE_PACKET_NUM			4
#define DATA_PACKET_NUM				3

#define DATUMDATA_LENGTH			5
#define IPADDRESS_LENGTH			4
#define ESP_PASSWORD_OFF			(DATUMDATA_LENGTH+IPADDRESS_LENGTH)

#define ESPTOUCH_ACK_UDPPORT		18266
#define STEP_MULTICAST_HOLD_CHANNEL		5

typedef enum{
	SMART_CH_INIT 		=	0x1,
	SMART_CH_LOCKING 	=	0x2,
	SMART_CH_LOCKED 	=	0x4,
	SMART_FINISH 		= 	0x8
}smnt_status_t;

typedef struct{
	smnt_status_t state;
	uint8 group_addr;
	uint8 packet_count;
	uint16 offset_len;
	uint16 packet_data[3];
	uint16 index;
	uint8 total_len;
	uint8 password_len;
	uint8 directTimerSkip;
	uint8 syncAppMac[6];
	uint8 syncBssid[6];
	uint8 chCurrentIndex;
	uint8 chCurrentProbability;
	uint8 payload_multicast[128];
}esptouch_Smnt_t;

static esptouch_Smnt_t *pSmnt = NULL;
static esptouch_smnt_param_t esptouch_smnt_gobal;

static void esptouch_smnt_muticastadd(uint8* pAddr, int length);
void esptouch_stop(void);

extern uint8_t getAvailableIndex(void);
extern void recordAP(void);
extern void colinkSettingStart(void);
//------------------------------------
static uint8 esptouch_smnt_crc(uint8 *ptr, uint8 len)
{
	unsigned char crc;
	unsigned char i;
	crc = 0;
	while (len--)
	{
		crc ^= *ptr++;
		for (i = 0; i < 8; i++)
		{
			if (crc & 0x01)
			{
				crc = (crc >> 1) ^ 0x8C;
			}
			else
				crc >>= 1;
		}
	}
	return crc;
}

void esptouch_smnt_init(esptouch_smnt_param_t *param)
{
	pSmnt = (esptouch_Smnt_t *)OS_MemAlloc(sizeof(esptouch_Smnt_t));

    memset(pSmnt, 0, sizeof(esptouch_Smnt_t));
	pSmnt->group_addr = 0xFF;
	return;
}

void esptouch_smnt_release(void)
{
	printf("%s\n", __func__);

	memset(&esptouch_smnt_gobal, 0, sizeof(esptouch_smnt_gobal));
	if(pSmnt != NULL){
		OS_MemFree(pSmnt);
		pSmnt = NULL;
	}
}

void esptouch_smnt_reset(void)
{
	if(pSmnt != NULL){
		memset(pSmnt,0,sizeof(pSmnt));
	}
}

static void esptouch_smnt_finish(void)
{
	int i;
	esptouch_smnt_result_t smnt_result;

	memset(&smnt_result,0,sizeof(smnt_result));
	if (pSmnt && (pSmnt->state == SMART_FINISH) ) 
	{
		smnt_result.esp_password_len = pSmnt->payload_multicast[1];
		smnt_result.esp_ssid_len	 = pSmnt->payload_multicast[0]-smnt_result.esp_password_len-ESP_PASSWORD_OFF;

		memcpy(smnt_result.esp_password, pSmnt->payload_multicast+ESP_PASSWORD_OFF, smnt_result.esp_password_len);
		for(i=0; i<smnt_result.esp_password_len; i++)
		{
			if(i & 0x01) smnt_result.esp_password[i] -= 2;
			else smnt_result.esp_password[i] -= 7;
		}
		smnt_result.esp_recv_len = pSmnt->payload_multicast[0];
		memcpy(smnt_result.esp_src_ip, pSmnt->payload_multicast+DATUMDATA_LENGTH, sizeof(smnt_result.esp_src_ip));
		//memcpy(smnt_result.esp_ssid, pSmnt->payload_multicast+ESP_PASSWORD_OFF+smnt_result.esp_password_len,smnt_result.esp_ssid_len);
		memcpy(smnt_result.esp_bssid_mac, pSmnt->syncBssid, sizeof(smnt_result.esp_bssid_mac));

		smnt_result.smnt_result_status = smnt_result_ok;
		if(esptouch_smnt_gobal.get_result_callback == NULL){
			printf("ERROR:esptouch_smnt_finish->get_result_callback NULL\n");
			return;
		}
		esptouch_smnt_gobal.get_result_callback(&smnt_result);
	}
}

int esptouch_smnt_cyclecall(void)
{
	if(pSmnt == NULL){
		return 50;
	}

	if(esptouch_smnt_gobal.switch_channel_callback == NULL){
		printf("switch channel function NULL\n");
		return 50;
	}

    if (pSmnt->directTimerSkip){
		pSmnt->directTimerSkip--;
		return 50;
	}

	if (pSmnt->state == SMART_FINISH){
		printf("-------------------->Finished\n");
		pSmnt->directTimerSkip = 10000/50;
		return 50;
	}

	if (pSmnt->chCurrentProbability > 0){
		pSmnt->chCurrentProbability--;
		//printf("------------------->SYNC (CH:%d) %d\n", pSmnt->chCurrentIndex, pSmnt->chCurrentProbability);
		return 50;
	}

	pSmnt->chCurrentIndex = (pSmnt->chCurrentIndex % 13) + 1;

	if(esptouch_smnt_gobal.switch_channel_callback != NULL){
		esptouch_smnt_gobal.switch_channel_callback(pSmnt->chCurrentIndex);
	}

	pSmnt->state = SMART_CH_LOCKING;
	pSmnt->chCurrentProbability = 0;
	printf("CH=%d, T=%d\n", pSmnt->chCurrentIndex, 50);
	return 50;
}

/*
Input: Muticast Addr
Output: -1:Unkown Packet, 0:Parse OK, 1:Normal Process
*/
static void esptouch_smnt_muticastadd(uint8* pAddr, int length)
{
	uint8 val_idx[2]; //value,index
	uint8 crc8, crc82;

	if (pAddr[3] != pAddr[4] || pAddr[3] != pAddr[5])
		return;

	length -= pSmnt->offset_len + ESPTOUCH_PACKET_SUB;
	if(length >= ESPTOUCH_GUIDE_CODE-3)
		return;

	if(pSmnt->group_addr == pAddr[3])
	{
		pSmnt->packet_data[pSmnt->packet_count] = length;
		pSmnt->packet_count++;
		if(pSmnt->packet_count == DATA_PACKET_NUM)
		{
			if(pSmnt->packet_data[1] == 0x100 + pSmnt->index) //sequence header
			{
				val_idx[0] = ((pSmnt->packet_data[0] << 4) & 0xF0) | (pSmnt->packet_data[2] & 0x0F);
				val_idx[1] = pSmnt->packet_data[1] & 0xFF;
				crc8 = (pSmnt->packet_data[0] & 0xF0) | ((pSmnt->packet_data[2] >> 4) & 0x0F);
				crc82 = esptouch_smnt_crc(val_idx, 2);
				if(crc8 == crc82)
				{
					printf("vaule=0x%02x, %3d  %3d  %3d\n", val_idx[0], pSmnt->packet_data[0], pSmnt->packet_data[1], pSmnt->packet_data[2]);

					pSmnt->payload_multicast[pSmnt->index] = val_idx[0];
					if(pSmnt->index == 0) pSmnt->total_len = val_idx[0];
					if(pSmnt->index == 1) pSmnt->password_len = val_idx[0];

					pSmnt->index++;
					if(pSmnt->index == pSmnt->password_len + ESP_PASSWORD_OFF)
					{
						pSmnt->state = SMART_FINISH;
						esptouch_smnt_finish();
					}
				}
			}
			pSmnt->group_addr = 0xFF;
		}
	}
	else
	{
		pSmnt->packet_data[0] = length;
		pSmnt->group_addr = pAddr[3];
		pSmnt->packet_count = 1;
	}

	if (pSmnt->chCurrentProbability < 20)
		pSmnt->chCurrentProbability += STEP_MULTICAST_HOLD_CHANNEL;			// Delay CH switch!
	return;
}

void esptouch_smnt_datahandler(PHEADER_802_11 pHeader, int length)
{
	uint8 isUplink = 1;
	uint8 packetType = 0;					// 1-multicast packets 2-broadcast packets 0-thers
	uint8 isDifferentAddr = 0;
	uint8 *pDest, *pSrc, *pBssid;

	if (pSmnt == NULL)
		return;
	if (/*((length > 100) && (pSmnt->state != SMART_CH_LOCKED)) || */pSmnt->state == SMART_FINISH)	
		return;

	if (pHeader->FC.ToDs)
	{
		isUplink = 1;
		pBssid = pHeader->Addr1;
		pSrc = pHeader->Addr2;
		pDest = pHeader->Addr3;

		if (!(/*(memcmp(pDest, MAC_BROADCAST, 6) == 0) || */(memcmp(pDest, MAC_MULTICAST, 3) == 0))){
			return;
		}
	}
	else
	{
		pDest = pHeader->Addr1;
		pBssid = pHeader->Addr2;
		pSrc  = pHeader->Addr3;

		isUplink = 0;
		//not broadcast nor multicast package ,return
		if (!(/*(memcmp(pDest, MAC_BROADCAST, 6) == 0) || */(memcmp(pDest, MAC_MULTICAST, 3) == 0))){
			return;
		}
	}
	if (memcmp(pDest, MAC_MULTICAST, 3) == 0)
	{
		//if (pSmnt->state == SMART_CH_LOCKING)
		//	printf("(%02x-%04d):%02x:%02x:%02x->%d\n", *((uint8*)pHeader) & 0xFF, pHeader->Sequence, pDest[3], pDest[4], pDest[5], length);
		packetType = 1;
	}

	if (memcmp(pSrc, pSmnt->syncAppMac, 6) != 0)
	{
		isDifferentAddr = 1;
	}

	if(pSmnt->state == SMART_CH_LOCKING)
	{
		if (packetType == 0) return;
		if(!isDifferentAddr)
		{
			if (packetType != 0)
			{
				if (packetType == 1 && length >= ESPTOUCH_GUIDE_CODE)
				{
					if (pDest[3] == pDest[4] && pDest[3] == pDest[5])
					{
						if(pSmnt->group_addr == pDest[3])
						{
							if(pSmnt->offset_len == pSmnt->packet_count + length) //guide_code:514/513/512
							{
								pSmnt->packet_count++;
								if(pSmnt->packet_count == GUIDE_PACKET_NUM)
								{
									if (pSmnt->chCurrentProbability < 20) pSmnt->chCurrentProbability = 10;

									memcpy(pSmnt->syncBssid, pBssid, sizeof(pSmnt->syncBssid));
									pSmnt->state = SMART_CH_LOCKED;

									printf("SMART_CH_LOCKED:%d\n", pSmnt->offset_len);

									pSmnt->offset_len -= ESPTOUCH_GUIDE_CODE;
									pSmnt->group_addr = 0xFF;
								}
							}
							else pSmnt->group_addr = 0xFF;
						}
						else
						{
							pSmnt->group_addr = pDest[3];
							pSmnt->packet_count = 1;
							pSmnt->offset_len = length; //guide_code:515
						}

					}
					if (pSmnt->chCurrentProbability < 20)
						pSmnt->chCurrentProbability += STEP_MULTICAST_HOLD_CHANNEL; // Delay CH switch!
					return;
				}
			}
			return;
		}
		memcpy(pSmnt->syncAppMac, pSrc, 6);
		printf("Try to SYNC!\n");
		return;
	}
	else if (pSmnt->state == SMART_CH_LOCKED)
	{
		if (isDifferentAddr) return;

		if (packetType == 1){
			esptouch_smnt_muticastadd(pDest, length);
			return;
		}
	}
	else if (pSmnt->state == SMART_FINISH)
	{
		printf("SMART_FINISH-1\n");
	}
	else
	{
		pSmnt->state = SMART_CH_LOCKING;
		memcpy(pSmnt->syncAppMac, pSrc, 6);
		printf("Reset All State\n");
	}
	return;
}

//////////////////////////////////////
#define CHANGE_CH_TIME	150

static esptouch_smnt_result_t esptouch_wifi_result;
static OsTimer ch_change_timer;
static int start = 0;
static uint8_t sniffer_ch;

//-------------------------------------
static void esptouch_ack_app(uint8_t *ipaddr, uint8_t *mac)
{
    int sockfd;
	uint8_t pdata[1+6+4];
    struct sockaddr_in sock_addr;

    /* create a UCP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
        printf("Failed to create socket:%d\n", sockfd);
        return;
    }

    pdata[0] = esptouch_wifi_result.esp_recv_len;
    memcpy(&pdata[1], mac, 6);
    memcpy(&pdata[7], ipaddr, 4);

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(ESPTOUCH_ACK_UDPPORT);
    sock_addr.sin_len = sizeof(sock_addr);
    sock_addr.sin_addr.s_addr = *(uint32_t*)esptouch_wifi_result.esp_src_ip;
	
    for(int i=0; i<10 ;i++)
    {
        if(sendto(sockfd, pdata, sizeof(pdata), 0, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0)
        {
            printf("Failed to create socket:%d\n", sockfd);
            return;
        }
        OS_MsDelay(50);
    }
}

static void esptouchwificbfunc(WIFI_RSP *msg)
{
    extern void wifi_status_cb(int connect);

    printf("=========\n");
    printf("%s wifistatus = %d\n", __func__, msg->wifistatus);
    
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

        recordAP();
        esptouch_ack_app(ipaddr.u8, mac);
        colinkSettingStart();
    }
    else
    {
        printf("Wifi Disconnect\n");
    }
    wifi_status_cb(msg->wifistatus);
}

static void esptouch_scan_cbfunc(void *data)
{
    int i, ret;
    printf("scan finish\n");

	if(esptouch_wifi_result.esp_ssid[0] == 0x00) //ssid is NULL
	{
		ret = getAvailableIndex();
		for(i = 0; i < ret; i++)
		{
			if(memcmp(ap_list[i].mac, esptouch_wifi_result.esp_bssid_mac, sizeof(esptouch_wifi_result.esp_bssid_mac)) == 0)
			{
				memcpy(esptouch_wifi_result.esp_ssid, ap_list[i].name, ap_list[i].name_len);
				esptouch_wifi_result.esp_ssid_len = ap_list[i].name_len;
				break;
			}
		}
	}
	if(esptouch_wifi_result.esp_ssid[0] == 0x00) //ssid is NULL
		return;

    ret = set_wifi_config(esptouch_wifi_result.esp_ssid, esptouch_wifi_result.esp_ssid_len, esptouch_wifi_result.esp_password, esptouch_wifi_result.esp_password_len, NULL, 0);
    if(ret == 0)
        ret = wifi_connect(esptouchwificbfunc);
}

static int esptouch_connect_ap(void *pResult)
{
    int ret = 0;

    memset(&esptouch_wifi_result, 0, sizeof(esptouch_wifi_result));
    memcpy(&esptouch_wifi_result, pResult, sizeof(esptouch_wifi_result));
    DUT_wifi_start(DUT_STA);
    scan_AP(esptouch_scan_cbfunc);

    return ret;
}

static void esptouch_get_result_cb(esptouch_smnt_result_t *result)
{
    int ret = 0;

    printf("==============================\n");
    printf("flag %d\n", result->smnt_result_status);
    printf("ssid %s, len %d\n", result->esp_ssid, result->esp_ssid_len);
    printf("pswd %s, len %d\n", result->esp_password, result->esp_password_len);
    printf("==============================\n");
    esptouch_stop();

    if( result->smnt_result_status == smnt_result_ok )
        esptouch_connect_ap(result);
}

static void esptouch_switch_ch_cb(unsigned char ch)
{
    set_channel((u8) ch);
}

static void ch_change_handler(void)
{
    esptouch_smnt_cyclecall();
    OS_TimerStart(ch_change_timer);
}

static int init_scan_ch_timer(void)
{
    printf("%s\n", __func__);
    if( OS_TimerCreate(&ch_change_timer, CHANGE_CH_TIME, (u8)FALSE, NULL, (OsTimerHandler)ch_change_handler) == OS_FAILED)
        return OS_FAILED;

    OS_TimerStart(ch_change_timer);
    return OS_SUCCESS;
}

static void sniffer_cb(packetinfo *packet)
{
    PHEADER_802_11 hdr = (PHEADER_802_11)packet->data;
    esptouch_smnt_datahandler(hdr, packet->len);
}

void esptouch_init(void)
{
    printf("%s\n", __func__);

    if (start)
        return;

	sniffer_ch = 0;
    attach_sniffer_cb(RECV_DATA_BCN, &sniffer_cb, 512);
    start_sniffer_mode();

    init_scan_ch_timer();
    esptouch_smnt_gobal.switch_channel_callback = &esptouch_switch_ch_cb;
    esptouch_smnt_gobal.get_result_callback = &esptouch_get_result_cb;
    memset(esptouch_smnt_gobal.secretkey, 0, sizeof(esptouch_smnt_gobal.secretkey));
    esptouch_smnt_init(&esptouch_smnt_gobal);
    start = 1;
}

void esptouch_stop(void)
{
    printf("%s\n", __func__);
    if (start) {
        if (ch_change_timer) {
            OS_TimerDelete(ch_change_timer);
            ch_change_timer = NULL;
        }
        deattach_sniffer_cb();
        stop_sniffer_mode();
        esptouch_smnt_release();
        start = 0;
    }
}

