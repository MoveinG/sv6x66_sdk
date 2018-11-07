//#include <config.h>
//#include <stdarg.h>
#include <stdio.h>
#include "osal.h"
#include "sntp/sntp.h"
#include "colink_setting.h"
#include "mytime.h"

#define USE_COLINK_UTC //no use sntp
#define SEC_TO201811	(1514764800) //second count 1970-01-01 00:00:00 to 2018-01-01 00:00:00
#define WEEK_197011		4 //0-sunday(0--6)
#define DAY_SECOND		(24*60*60) //second count for 1 DAY
#define IS_MON2DAY(n)	((n%4) ? 0 : ((n%100) ? 1 : ((n%400) ? 0 : 1)))
#define YEAR366NUM(n)	(((n - 1968) / 4) - ((n - 1900) / 100) + ((n - 1600) / 400)) //y2000 is 366, y2100, y2200, y2300 is 365

//static const signed char day_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const signed short day_month[13] = {0, 31, 59, 90, 120,151,181,212,243,273,304,334,365};

#define SNTP_TIME_MS	(1000)
#define SNTP_UPDATE_MS	(24*3600*1000) //1-day
#define TICK_OVER_SEC	(4294967) //0x100000000/1000(s) + 296(ms)

#define MYTIME_IS_IDLE	0
#define MYTIME_SNTPING	1
#define MYTIME_SNTPED	2
#define MYTIME_UPDATE	4
#define MYTIME_DELAY	16
#define MYTIME_1WEEK	32

static OsTimer sntp_update_timer=NULL;
static OsTimer mytime_delay_timer=NULL;

static colink_app_timer *ap_timer_list=NULL;
static int timer_num;

static unsigned int prev_tick=0;
static unsigned int realtime_offset=SEC_TO201811; //2018-01-01 00:00:00
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

#ifndef USE_COLINK_UTC
static void sntp_update_handler(void)
{
	if(mytime_state == MYTIME_SNTPING)
	{
		unsigned int offset = psGetTime(NULL, NULL);
		if(offset != 0)
		{
			prev_tick = OS_GetSysTick();
			realtime_offset = offset - os_tick2ms(prev_tick) / 1000;
			mytime_state = MYTIME_SNTPED;

			printf("%s realtime_offset=%d\n", __func__, realtime_offset);

			Timer_update_time();
			//OS_TimerSet(sntp_update_timer, SNTP_UPDATE_MS, (unsigned char)FALSE, NULL);
		}
		else OS_TimerStart(sntp_update_timer);
	}
	/*else
	{
		unsigned int tick = OS_GetSysTick();
		if(prev_tick > tick) realtime_offset += os_tick2ms(TICK_OVER_SEC);
		prev_tick = tick;

		//tick = psGetTime(NULL, NULL);
		//tick -= realtime_offset + os_tick2ms(OS_GetSysTick()) / 1000;
		//printf("%s offset=%d\n", __func__, tick);
 	}*/
}
#endif

static void mytime_delay_handler(void)
{
	printf("%s state=%d, do_switch=%d\n", __func__, mytime_state, do_switch);

	if(ap_timer_list == NULL) return;
	if(mytime_state == MYTIME_DELAY)
	{
		Timer_Switch_cb(do_switch ? 1 : 0);
	}
	Timer_update_time(); //if(mytime_state == MYTIME_1WEEK)
}

void mytime_start(void)
{
	if(mytime_state != MYTIME_IS_IDLE)
		return;

	colink_load_timer((char**)&ap_timer_list);
	mytime_state = MYTIME_SNTPING;

	#ifndef USE_COLINK_UTC
	sntp_init();

	OS_TimerCreate(&sntp_update_timer, SNTP_TIME_MS, (signed char)FALSE, NULL, (OsTimerHandler)sntp_update_handler);
	if(sntp_update_timer) OS_TimerStart(sntp_update_timer);
	else printf("!! TimerCreate sntp_update_timer failed\n");
	#endif

	OS_TimerCreate(&mytime_delay_timer, SNTP_UPDATE_MS, (signed char)FALSE, NULL, (OsTimerHandler)mytime_delay_handler);
	if(mytime_delay_timer == NULL) printf("!! TimerCreate mytime_delay_timer failed\n");
}

void mytime_stop(void)
{
	mytime_state = MYTIME_IS_IDLE;

	if(ap_timer_list)
	{
		OS_MemFree(ap_timer_list);
		ap_timer_list = NULL;
	}

	#ifndef USE_COLINK_UTC
	sntp_stop();
	if(sntp_update_timer)
	{
		OS_TimerDelete(sntp_update_timer);
		sntp_update_timer = NULL;
	}
	#endif

	if(mytime_delay_timer)
	{
		OS_TimerDelete(mytime_delay_timer);
		mytime_delay_timer = NULL;
	}
}

unsigned int mytime_get_time(struct mydatetime *ptime)
{
	unsigned int value;

	OS_EnterCritical();
	value = OS_GetSysTick();
	if(prev_tick > value) realtime_offset += os_tick2ms(TICK_OVER_SEC);
	prev_tick = value;
	OS_ExitCritical();

	value = realtime_offset + os_tick2ms(OS_GetSysTick()) / 1000;
	if(ptime)
	{
		*ptime = second_to_date(value);
	}
	return value;
}

void colink_UTC_str(const char *timestr)
{
	unsigned int value;

	value = mytime_str_to_time(timestr);
	if(value != 0)
	{
		OS_EnterCritical();
		prev_tick = OS_GetSysTick();
		realtime_offset = value - os_tick2ms(prev_tick) / 1000;
		OS_ExitCritical();

		#ifdef USE_COLINK_UTC
		if(mytime_state == MYTIME_SNTPING)
		{
			OS_EnterCritical();
			mytime_state = MYTIME_SNTPED;
			OS_ExitCritical();

			Timer_update_time();
			//OS_TimerSet(sntp_update_timer, SNTP_UPDATE_MS, (unsigned char)FALSE, NULL);
			//OS_TimerStart(sntp_update_timer);
		}
		#endif
	}
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

static unsigned int get_min_time(const colink_app_timer *ap_time, int num)
{
	unsigned int cur_time, off_time;
	unsigned char cur_switch;

	cur_switch = 0xFF;
	off_time = 0xFFFFFFFF; //max time
	cur_time = mytime_get_time(NULL);
	for(int i=0; i<num; i++)
	{
		if(ap_time->type == COLINK_TYPE_ONCE)
		{
			if(ap_time->at_time == cur_time)
				cur_switch = ap_time->start_do;

			if(ap_time->at_time > cur_time && off_time > ap_time->at_time) 
			{
				off_time = ap_time->at_time;
				do_switch = ap_time->start_do;
			}
		}
		if(ap_time->type == COLINK_TYPE_REPEAT)
		{
			if(ap_time->cron.week_bit)
			{
				int i, wday, cur_day;
				unsigned int value;

				cur_day = cur_time / DAY_SECOND; //day
				wday = (cur_day + WEEK_197011) % 7; //today at week?
				i = wday;
				while((i - wday) < 8)
				{
					if(ap_time->cron.week_bit & (1 << (i % 7)))
					{
						value = DAY_SECOND * (cur_day + i - wday) + ap_time->cron.hour * 3600 + ap_time->cron.min * 60;
						printf("%s %d, value=%d, hour=%d, min=%d\n", __func__, i, value, ap_time->cron.hour, ap_time->cron.min);

						if(value == cur_time) cur_switch = ap_time->start_do;
						if(value > cur_time)
						{
						 	if(off_time > value)
							{
								off_time = value;
								do_switch = ap_time->start_do;
							}
							break;
						}
					}
					i++;
				}
			}
		}
		if(ap_time->type == COLINK_TYPE_DURATION)
		{
			unsigned int value, cycle, delay;

			cycle = ap_time->cycle * 60; //to secnond
			if(ap_time->at_time > cur_time)
			{
				if(off_time > ap_time->at_time) 
				{
					off_time = ap_time->at_time;
					do_switch = ap_time->start_do;
				}
			}
			else if(ap_time->at_time < cur_time)
			{
				value = ap_time->at_time + ((cur_time - ap_time->at_time) / cycle) * cycle;
				if(value == cur_time) cur_switch = ap_time->start_do;

				delay = value + ap_time->delay * 60;
				if(delay == cur_time) cur_switch = ap_time->end_do;
				if(off_time > delay)
				{
					off_time = delay;
					do_switch = ap_time->end_do;
				}

				cycle += value;
				if(cycle == cur_time) cur_switch = ap_time->start_do;
				if(off_time > cycle)
				{
					off_time = cycle;
					do_switch = ap_time->start_do;
				}
				printf("%s cycle+1=%d, delay=%d\n", __func__, cycle, delay);
			}
			else cur_switch = ap_time->start_do;
		}
		ap_time++;
	}

	if(cur_switch != 0xFF) Timer_Switch_cb(cur_switch ? 1 : 0);

	printf("%s time=%d - %d\n", __func__, off_time, cur_time);
	return off_time;
}

void *mytime_get_buffer(int size)
{
	if(ap_timer_list != NULL) OS_MemFree(ap_timer_list);

	timer_num = size / sizeof(colink_app_timer);
	ap_timer_list = OS_MemAlloc(size);

	printf("%s ap_timer_list=%x num=%d\n", __func__, (unsigned int)ap_timer_list, timer_num);
	return ap_timer_list;
}

int mytime_update_delay(void)
{
	if(ap_timer_list == NULL || timer_num <= 0)
		return 0;
	if(mytime_state == MYTIME_IS_IDLE || mytime_state == MYTIME_SNTPING)
		return 0;

	unsigned int diff_time = get_min_time(ap_timer_list, timer_num);
	if(diff_time == 0xFFFFFFFF)
	{
		mytime_state = MYTIME_SNTPED;
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

void mytime_clean_delay(void)
{
	if(ap_timer_list)
	{
		OS_MemFree(ap_timer_list);
		ap_timer_list = NULL;
	}

	if(mytime_delay_timer) OS_TimerStop(mytime_delay_timer);

	colink_delete_timer();
}

