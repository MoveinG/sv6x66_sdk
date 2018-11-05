#ifndef __COLINK_MYTIME_H__
#define __COLINK_MYTIME_H__

#include "colink_network.h"

struct mydatetime {
	unsigned short year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char wday;
};

void mytime_start(void);
void mytime_stop(void);

unsigned int mytime_get_time(struct mydatetime *ptime);
unsigned int mytime_str_to_time(const char *timestr);

cron_lite mytime_str_repeat(const char *cronstr);

void *mytime_get_buffer(int size);
int mytime_update_delay(void);
void mytime_clean_delay(void);

void colink_UTC_str(const char *timestr);

#endif
