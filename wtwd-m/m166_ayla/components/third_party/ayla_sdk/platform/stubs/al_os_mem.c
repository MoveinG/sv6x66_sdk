/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_os_mem.h>

void al_os_mem_set_type(enum al_os_mem_type type)
{
}

enum al_os_mem_type al_os_mem_get_type(void)
{
	return al_os_mem_type_long_period;
}

void *al_os_mem_alloc(size_t size)
{
	return NULL;
}

void *al_os_mem_calloc(size_t size)
{
	return NULL;
}

void al_os_mem_free(void *mem)
{
}
