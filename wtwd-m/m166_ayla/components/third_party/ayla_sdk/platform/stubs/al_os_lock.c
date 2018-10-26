/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_os_lock.h>

struct al_lock {
	int lock;
};

struct al_lock *al_os_lock_create(void)
{
	return NULL;
}

void al_os_lock_lock(struct al_lock *lock)
{
}

void al_os_lock_unlock(struct al_lock *lock)
{
}

void al_os_lock_destroy(struct al_lock *lock)
{
}
