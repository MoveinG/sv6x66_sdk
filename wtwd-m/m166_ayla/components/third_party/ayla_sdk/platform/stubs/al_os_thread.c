/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_os_thread.h>

struct al_thread {
	u32	exitcode;
};

struct al_thread *al_os_thread_create(const char *name,
	void *stack, size_t stack_size, enum al_os_thread_pri pri,
	void (*thread_main)(struct al_thread *thread, void *arg), void *arg)
{
	return NULL;
}

int al_os_thread_suspend(struct al_thread *thread)
{
	return 0;
}

int al_os_thread_resume(struct al_thread *thread)
{
	return 0;
}

enum al_os_thread_pri al_os_thread_get_priority(struct al_thread *thread)
{
	return al_os_thread_pri_normal;
}

void al_os_thread_set_priority(struct al_thread *thread,
	enum al_os_thread_pri pri)
{
}

void al_os_thread_set_exit_code(struct al_thread *thread, u32 code)
{
}

int al_os_thread_get_exit_flag(struct al_thread *thread)
{
	return 1;
}

void al_os_thread_terminate(struct al_thread *thread)
{
}

u32 al_os_thread_terminate_with_status(struct al_thread *thread)
{
	return 0;
}

u32 al_os_thread_join(struct al_thread *thread)
{
	return 0;
}

struct al_thread *al_os_thread_self(void)
{
	return NULL;
}

void al_os_thread_free(struct al_thread *thread)
{
}

void al_os_thread_sleep(int ms)
{
}
