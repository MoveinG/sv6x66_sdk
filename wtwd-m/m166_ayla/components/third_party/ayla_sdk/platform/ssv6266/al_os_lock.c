/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_os_lock.h>
#include "osal.h"

struct al_lock {
	int lock;
};

struct al_lock *al_os_lock_create(void)
{
	struct al_lock *p_mtx = al_os_mem_alloc(sizeof(struct al_lock));
	if(p_mtx != NULL){
		OS_MutexInit(&p_mtx->lock);
		return p_mtx;
	}
	return NULL;
}

void al_os_lock_lock(struct al_lock *lock)
{
	OS_MutexLock(lock->lock);
}

void al_os_lock_unlock(struct al_lock *lock)
{
	OS_MutexUnLock(lock->lock);
}

void al_os_lock_destroy(struct al_lock *lock)
{
	OS_MutexDelete(lock->lock);
	OS_MemFree(lock);
}
