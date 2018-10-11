//#include <config.h>
//#include <stdarg.h>
#include <stdio.h>
#include "osal.h"
#include "sntp/sntp.h"
#include "colink_setting.h"
#include "mytime.h"

//#define SEC_TO201811	1514764800 //second count 1970-01-01 00:00:00 to 2018-01-01 00:00:00
#define WEEK_197011	4 //0-sunday(0--6)	
#define DAY_SECOND	(24*60*60) //second count for 1 DAY
#define IS_MON2DAY(n)	((n%4) ? 0 : ((n%100) ? 1 : ((n%400) ? 0 : 1)))
#define YEAR366NUM(n)	(((n - 1968) / 4) - ((n - 1900) / 100) + ((n - 1600) / 400)) //y2000 is 366, y2100, y2200, y2300 is 365

//static const signed char day_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const signed short day_month[13] = {0, 31, 59, 90, 120,151,181,212,243,273,304,334,365};

#define SNTP_TIME_MS	(1000)
#define SNTP_UPDATE_MS	(24*3600*1000) //1-day

#define MYTIME_IS_IDLE	0
#define MYTIME_SNTPING	1
#define MYTIME_SNTPED	2
#define MYTIME_DELAY	3
#define MYTIME_1WEEK	4

static OsTimer sntp_update_timer=NULL;
static OsTimer mytime_delay_timer=NULL;

static colink_app_timer *app_timer=NULL;
static int timer_num;

static unsigned int realtime_offset=0;
static unsigned char mytime_state=MYTIME_IS_IDLE;
static unsigned char do_switch;

extern int32_t psGetTime(void *t, void *userPtr);
extern void Timer_Switch_cb(int open);
extern void Timer_update_time(void);

/////////////////////////////////////////////////////
static unsigned int date_to_second(struct mydatetime date)
{
	unsigned int value;

	if(date.year < 1970 || date.mon < 1 || date.mon > 12)
		return 0;

	value = (date.year - 1970) * 365;
	value += YEAR366NUM(date.year);

	value += day_month[date.mon - 1];
	if(IS_MON2DAY(date.year) != 0 && date.mon  <= 2)
		value--;

	value += date.day - 1;
	value *= DAY_SECOND;
	value += date.hour * 3600 + date.min * 60 + date.sec;

	return value;
}

static struct mydatetime second_to_date(unsigned int second)
{
	struct mydatetime date;
	int value, day, i;

	value = second % DAY_SECOND;
	date.hour = value / 3600;
	value = value % 3600;
	date.min = value / 60;
	date.sec = value % 60;

	day = second / DAY_SECOND;
	date.wday = (day + WEEK_197011) % 7;
#if 0
	date.year = 1970 + (day / 365);
	day = day % 365;

	value = YEAR366NUM(date.year-1);
	if(day < value)
	{
		date.year--;
		day += 365 - value;
		value = IS_MON2DAY(date.year);
		if(value) day++;
	}
	else
	{
		day -= value;
		value = IS_MON2DAY(date.year);
	}
#else
	i = 1970 + (day / 365);
	day -= YEAR366NUM(1969 + (day / 365));

	date.year = 1970 + (day / 365);
	day = day % 365;

	value = IS_MON2DAY(date.year);
	if(value && i != date.year) day++;
#endif

	for(i=0; i<13; i++)
	{
		if(day < day_month[i]) break;
	}
	date.mon = i;

	day -= day_month[i-1];
	if(i > 2 && value) day--;

	if(day < 0)
	{
		date.mon--;
		day += day_month[date.mon] - day_month[date.mon-1];
		if(date.mon == 2) day++;
	}
	date.day = day + 1;

	return date;
}

static void sntp_update_handler(void)
{
	if(mytime_state == MYTIME_SNTPING)
	{
		realtime_offset = psGetTime(NULL, NULL);
		if(realtime_offset != 0)
		{
			realtime_offset -= os_tick2ms(OS_GetSysTick()) / 1000;
			mytime_state = MYTIME_SNTPED;
			printf("%s realtime_offset=%d\n", __func__, realtime_offset);

			Timer_update_time();

			OS_TimerSet(sntp_update_timer, 3600*1000, (unsigned char)FALSE, NULL);
		}
	}
	else
	{
		unsigned int realtime = psGetTime(NULL, NULL);

		realtime -= realtime_offset + os_tick2ms(OS_GetSysTick()) / 1000;

		printf("%s offset=%d\n", __func__, realtime);
	}

	OS_TimerStart(sntp_update_timer);
}

static void mytime_delay_handler(void)
{
	printf("%s state=%d, do_switch=%d\n", __func__, mytime_state, do_switch);

	if(app_timer == NULL) return;
	if(mytime_state == MYTIME_DELAY)
	{
		Timer_Switch_cb(do_switch ? 1 : 0);
	}
	Timer_update_time(); //if(mytime_state == MYTIME_1WEEK)
}

void mytime_start(void)
{
	if(app_timer)
	{
		OS_MemFree(app_timer);
		app_timer = NULL;
	}
	colink_load_timer((char**)&app_timer);

	if(mytime_state != MYTIME_IS_IDLE)
		return;

	sntp_init();
	mytime_state = MYTIME_SNTPING;

	realtime_offset = psGetTime(NULL, NULL);
	printf("psGetTime=%d\n", realtime_offset);

	OS_TimerCreate(&sntp_update_timer, SNTP_TIME_MS, (signed char)FALSE, NULL, (OsTimerHandler)sntp_update_handler);
	if(sntp_update_timer) OS_TimerStart(sntp_update_timer);
	else printf("!! TimerCreate sntp_update_timer failed\n");

	OS_TimerCreate(&mytime_delay_timer, SNTP_UPDATE_MS, (signed char)FALSE, NULL, (OsTimerHandler)mytime_delay_handler);
	if(mytime_delay_timer == NULL) printf("!! TimerCreate mytime_delay_timer failed\n");
}

void mytime_stop(void)
{
	realtime_offset =0;
	mytime_state = MYTIME_IS_IDLE;

	if(app_timer)
	{
		OS_MemFree(app_timer);
		app_timer = NULL;
	}
	if(sntp_update_timer)
	{
		OS_TimerDelete(sntp_update_timer);
		sntp_update_timer = NULL;
	}
	if(mytime_delay_timer)
	{
		OS_TimerDelete(mytime_delay_timer);
		mytime_delay_timer = NULL;
	}
}

unsigned int mytime_get_time(struct mydatetime *ptime)
{
	unsigned int value;

	value = realtime_offset + os_tick2ms(OS_GetSysTick()) / 1000;
	if(ptime)
	{
		*ptime = second_to_date(value);
	}
	return value;
}

//"2018-10-08T05:46:00.000Z"
unsigned int mytime_str_to_time(const char *timestr)
{
	struct mydatetime date;

	//sscanf(timestr, "%hd-%hd-%hdT%hd:%hd:%hd.%*dZ", );
	date.year = atoi(timestr);
	date.mon = atoi(timestr+5);
	date.day = atoi(timestr+8);
	date.hour = atoi(timestr+11);
	date.min = atoi(timestr+14);
	date.sec = atoi(timestr+17);
	date.wday = 0;
	return date_to_second(date);
}

//"59 8 * * 1,3,5,6"
cron_lite mytime_str_repeat(const char *cronstr)
{
	cron_lite cron;
	const char *p, *p1=cronstr;

	cron.sec = 0;
	p = cronstr;
	while(*p != 0)
	{
		if(*p == 0x20)
		{
			if(p1 == cronstr) cron.min = atoi(p1);
			else cron.hour = atoi(p1);
			p1 = p+1;
		}
		if(*p == '*')
		{
			p += 4;
			break;
		}
		p++;
	}
	p1 = p;
	cron.week_bit = 0;
	while(*p != 0)
	{
		if(*p == ',')
		{
			cron.week_bit |= 1 << atoi(p1);
			p1 = p+1;
		}
		p++;
	}
	cron.week_bit |= 1 << atoi(p1);

	return cron;
}

static unsigned int get_min_time(colink_app_timer *timer, int num)
{
	unsigned int cur_time, off_time;

	off_time = 0xFFFFFFFF; //max time
	cur_time = mytime_get_time(NULL);
	for(int i=0; i<num; i++)
	{
		if(timer->type == COLINK_TYPE_ONCE)
		{
			if(timer->at_time > cur_time && off_time > timer->at_time) 
			{
				off_time = timer->at_time;
				do_switch = timer->start_do;
			}
		}
		if(timer->type == COLINK_TYPE_REPEAT)
		{
			if(timer->cron.week_bit)
			{
				int value, i, wday, cur_day;

				cur_day = cur_time / DAY_SECOND; //day
				wday = (cur_day + WEEK_197011) % 7; //today at week?
				i = wday;
				while((i - wday) < 8)
				{
					if(timer->cron.week_bit & (1 << (i % 7)))
					{
						value = DAY_SECOND * (cur_day + i - wday) + timer->cron.hour * 3600 + timer->cron.min * 60;
						printf("%s %d, value=%d, hour=%d, min=%d\n", __func__, i, value, timer->cron.hour, timer->cron.min);
						if(value > cur_time)
						{
						 	if(off_time > value)
							{
								off_time = value;
								do_switch = timer->start_do;
							}
							break;
						}
					}
					i++;
				}
			}
		}
		timer++;
	}
	printf("%s time=%d - %d\n", __func__, off_time, cur_time);
	return off_time;
}

void *mytime_get_buffer(int size)
{
	if(app_timer != NULL) OS_MemFree(app_timer);

	timer_num = size / sizeof(colink_app_timer);
	app_timer = OS_MemAlloc(size);

	printf("%s app_timer=%x num=%d\n", __func__, (unsigned int)app_timer, timer_num);
	return app_timer;
}

int mytime_update_delay(void)
{
	if(app_timer == NULL || timer_num <= 0)
		return 0;
	if(mytime_state == MYTIME_IS_IDLE || mytime_state == MYTIME_SNTPING)
		return 0;

	unsigned int diff_time = get_min_time(app_timer, timer_num);
	if(diff_time == 0xFFFFFFFF)
	{
		OS_MemFree(app_timer);
		app_timer = NULL;

		mytime_state = MYTIME_SNTPED;
		OS_TimerStop(mytime_delay_timer);
		return -1;
	}

	diff_time -= realtime_offset + os_tick2ms(OS_GetSysTick()) / 1000;
	if(diff_time > 10 * DAY_SECOND)
	{
		mytime_state = MYTIME_1WEEK;
		OS_TimerSet(mytime_delay_timer, 7*24*3600*1000, (unsigned char)FALSE, NULL);
	}
	else
	{
		mytime_state = MYTIME_DELAY;
		OS_TimerSet(mytime_delay_timer, diff_time*1000, (unsigned char)FALSE, NULL); //ms
	}
	OS_TimerStart(mytime_delay_timer);
	return 0;
}

