#ifndef __COLINK_NETWORK_H__
#define __COLINK_NETWORK_H__

#define IOTGO_NETWORK_CENTER 0

void network_init(void);
void colinkProcessStart(void);
void enterSettingSelfAPMode(void);
void colinkSoftOverStart(void);

#define COLINK_TYPE_ONCE		1
#define COLINK_TYPE_REPEAT		2
#define COLINK_TYPE_DURATION	3

#define COLINK_DO_ON		1
#define COLINK_DO_OFF		0

typedef struct
{
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t week_bit;
} cron_lite;

typedef struct
{
	uint8_t enable;
	uint8_t type;
	uint8_t start_do;
	uint8_t end_do;
	union {
		uint32_t at_time;
		cron_lite cron;
	};
	uint16_t min_b;
	uint16_t min_c;
} colink_app_timer;

//#define COLINK_SSL at colink_socket.c
#define SSLUSERENABLE

#endif /* #ifndef __COLINK_NETWORK_H__ */
