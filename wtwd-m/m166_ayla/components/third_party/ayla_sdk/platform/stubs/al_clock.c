/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_clock.h>

void al_clock_init(void)
{
}

void al_clock_final(void)
{
}

int al_clock_set(u32 timestamp, enum al_clock_src clock_src)
{
	return -1;
}

u32 al_clock_get(enum al_clock_src *clock_src)
{
	return 0;
}

u64 al_clock_get_total_ms(void)
{
	return 0;
}
