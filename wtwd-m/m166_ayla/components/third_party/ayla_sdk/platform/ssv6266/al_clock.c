/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_clock.h>
#include "osal.h"
#include <al/al_assert.h>

#define THOUSAND 1000

/* Local globals */
static s64 clock_at_boot;	/* ms since Epoch */
static s64 clock_of_adjust;	/* ms */
static enum al_clock_src gv_clock_src;
static struct al_lock *gp_clock_mutex;

struct timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
};

static void utc_clock_get(struct timeval *ptv)
{
	long cnt = os_tick2ms(OS_GetSysTick()*1000);
	ptv->tv_sec = cnt/1000000+1539772551;
	ptv->tv_usec = cnt%1000000;
	
}

void al_clock_init(void)
{
	struct timeval now;

	gp_clock_mutex = al_os_lock_create();
	al_os_lock_lock(gp_clock_mutex);
	utc_clock_get(&now);
	gv_clock_src = AL_CS_NONE;
	clock_at_boot = (s64)now.tv_sec * THOUSAND
	    + now.tv_usec / THOUSAND;
	clock_of_adjust = 0;
	al_os_lock_unlock(gp_clock_mutex);
}

void al_clock_final(void)
{
	al_os_lock_destroy(gp_clock_mutex);
	gp_clock_mutex = NULL;
}

int al_clock_set(u32 timestamp, enum al_clock_src clock_src)
{
	struct timeval now;
	s64 ct_now;
	s64 ct_new;

	if (clock_src < gv_clock_src ||
	    clock_src >= AL_CS_LIMIT) {
		return -1;
	}
	// ASSERT(gp_clock_mutex);
	al_os_lock_lock(gp_clock_mutex);
	gv_clock_src = clock_src;
	utc_clock_get(&now);
	ct_now = (s64)now.tv_sec * THOUSAND + now.tv_usec / THOUSAND;
	ct_new = (s64)timestamp * THOUSAND;
	clock_of_adjust = ct_new - ct_now;
	al_os_lock_unlock(gp_clock_mutex);
	return 0;
}

void al_clock_reset_src(void)
{
	gv_clock_src = AL_CS_NONE;
}

u32 al_clock_get(enum al_clock_src *clock_src)
{
	struct timeval now;
	s64 ct;

	// ASSERT(gp_clock_mutex);
	al_os_lock_lock(gp_clock_mutex);
	utc_clock_get(&now);
	if (clock_src) {
		*clock_src = gv_clock_src;
	}
	ct = now.tv_sec + (now.tv_usec / THOUSAND
		+ clock_of_adjust) / THOUSAND;
	al_os_lock_unlock(gp_clock_mutex);
	return (u32)ct;
}

u64 al_clock_get_total_ms(void)
{
	s64 ms;
	struct timeval now;

	// ASSERT(gp_clock_mutex);
	al_os_lock_lock(gp_clock_mutex);
	utc_clock_get(&now);
	ms = (s64)now.tv_sec * THOUSAND
	    + now.tv_usec / THOUSAND;
	ms = ms - clock_at_boot;
	al_os_lock_unlock(gp_clock_mutex);
	return ms;
}
