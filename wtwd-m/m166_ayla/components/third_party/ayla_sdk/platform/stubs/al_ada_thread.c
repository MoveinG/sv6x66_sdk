/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_ada_thread.h>

void al_ada_main_loop(void)
{
}

void al_ada_wakeup(void)
{
}

void al_ada_kill(void)
{
}

void al_ada_callback_init(struct al_ada_callback *cb,
		void (*func)(void *), void *arg)
{
}

void al_ada_call(struct al_ada_callback *cb)
{
}

void al_ada_sync_call(struct al_ada_callback *cb)
{
}

void al_ada_timer_set(struct timer *timer, unsigned long ms)
{
}

void al_ada_timer_cancel(struct timer *timer)
{
}
