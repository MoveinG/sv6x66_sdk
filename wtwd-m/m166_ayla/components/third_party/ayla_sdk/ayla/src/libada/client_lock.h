/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_CLIENT_LOCK_H__
#define __AYLA_CLIENT_LOCK_H__

static inline void client_lock(void)
{
}

static inline void client_unlock(void)
{
}

static inline void client_lock_stamp(const char *func, int line)
{
}

#define client_locked 1			/* satisfy ASSERTs */

static inline void notify_lock(void)
{
}

static inline void notify_unlock(void)
{
}

static inline void notify_lock_init(void)
{
}

#endif /* __AYLA_CLIENT_LOCK_H__ */
