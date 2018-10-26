/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_os_thread.h>
#include "osal.h"

struct al_thread {
	u32	exitcode;
	OsTaskHandle handler;
	void *arg;
};

struct al_thread *al_os_thread_create(const char *name,
	void *stack, size_t stack_size, enum al_os_thread_pri pri,
	void (*thread_main)(void *arg), void *arg)
{
	int ret = 0;
	if(stack_size == 0){
		stack_size = 1024*2;
	}
	struct al_thread *p_thread = OS_MemAlloc(sizeof(struct al_thread));
	if(p_thread == NULL){
		printf("Memalloc error!!!!\n");
		return NULL;
	}
	p_thread->arg = arg;
	ret = OS_TaskCreate(thread_main, name, stack_size, p_thread, (OS_TASK_PRIO3 - pri), &p_thread->handler);
	return p_thread;
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
	thread->exitcode = code;
}

int al_os_thread_get_exit_flag(struct al_thread *thread)
{
	return 0;
	// return thread->exitcode;
}

void al_os_thread_terminate(struct al_thread *thread)
{
	OS_TaskDelete(thread->handler);
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
	OS_MemFree(thread);
}

void al_os_thread_sleep(int ms)
{
	OS_MsDelay(ms);
}
