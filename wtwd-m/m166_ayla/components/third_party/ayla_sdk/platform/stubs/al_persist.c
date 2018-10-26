/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_persist.h>

enum al_err al_persist_data_write(enum al_persist_section section,
				const char *name,
				const void *buf, size_t len)
{
	return -1;
}

ssize_t al_persist_data_read(enum al_persist_section section,
			const char *name, void *buf, size_t len)
{
	return -1;
}

enum al_err al_persist_data_erase(enum al_persist_section section)
{
	return -1;
}
