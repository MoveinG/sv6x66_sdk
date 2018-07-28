#ifndef __COLINK_NETWORK_H__
#define __COLINK_NETWORK_H__

#define IOTGO_NETWORK_CENTER 0

void network_init(void);
void colinkProcessStart(void);
void enterSettingSelfAPMode(void);
void colinkSoftOverStart(void);

//#define COLINK_SSL at colink_socket.c
//#define SSLUSERENABLE

#endif /* #ifndef __COLINK_NETWORK_H__ */
