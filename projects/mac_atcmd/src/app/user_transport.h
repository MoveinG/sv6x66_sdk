#ifndef _USER_TRANSPORT_H_
#define _USER_TRANSPORT_H_




#define BUFFER_SIZE_MAX				100




typedef struct {
	WIFI_OPMODE deviceMode;
	char deviceConApStatus;
	char deviceConServerStatus;
	int  socketClientId;
	char socketClientChn;
	char socketClientCreateFlag;
	char socketClientRevFlag;
	char socketClientRevBuffer[BUFFER_SIZE_MAX];
	int  socketServerId;
	char socketSendAckFlag;
	char socketServerCreateFlag;
	char socketServerRevFlag;
	char socketServerRevBuffer[BUFFER_SIZE_MAX];
}device_status_t;




void user_transport_init(WIFI_OPMODE mode);
void user_transport_func(void);

int  connect_to_wifi(void);

void set_device_mode(WIFI_OPMODE mode);
WIFI_OPMODE get_device_mode(void);


void set_device_connect_ap_status(unsigned char status);
int  get_device_connect_ap_status(void);

void set_connect_server_status(unsigned char status);
int  get_connect_server_status(void);

void set_wifi_config_msg(int value);
int  get_wifi_config_msg(void);

void set_socket_send_ack(char value);
char get_socket_send_ack(void);

void set_rev_server_data_flag(int value);
int  get_rev_server_data_flag(void);

void get_wifi_param(char* wifiSsid,char* wifiKey);

extern device_status_t deviceStatus;



#endif

