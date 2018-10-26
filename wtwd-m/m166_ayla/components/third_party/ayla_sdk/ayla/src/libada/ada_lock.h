/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __ADA_LOCK_H__
#define __ADA_LOCK_H__

struct ada_lock;	/* semi-opaque, will be cast to an os-specific lock */

static inline struct ada_lock *ada_lock_create(const char *name)
{
	return (struct ada_lock *)(1);
}

static inline void ada_lock(struct ada_lock *lock)
{
}

static inline void ada_unlock(struct ada_lock *lock)
{
}

#endif /* __ADA_LOCK_H__ */
