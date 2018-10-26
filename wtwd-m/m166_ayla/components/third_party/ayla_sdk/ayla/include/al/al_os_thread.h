/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_OS_THREAD_H__
#define __AYLA_AL_COMMON_OS_THREAD_H__

#include <ayla/utypes.h>

/**
 * @file
 * Platform OS thread Interfaces
 */

/**
 * Opaque structure representing a OS thread.
 */
struct al_thread;

/**
 * Thread priority.
 */
enum al_os_thread_pri {
	al_os_thread_pri_high,		/**< High priority */
	al_os_thread_pri_normal,	/**< Normal priority */
	al_os_thread_pri_low,		/**< Low priority */
};

/**
 * Create a thread
 *
 * \param name is the thread name.
 * \param stack is the memory of the stack, it's NULL to allocate the memory
 * from the heap.
 * \param stack_size is the stack size in bytes.
 * \param pri is the thread priority.
 * \param thread_main is a function of the main entry of the thread, It's
 * parameters are the thread, a pointer to the thread, and the arg is passed
 * from the arg of al_os_thread_create.
 * \param arg is the parameter is passed to the thread_main.
 *
 * \returns pointer to the thread structure or NULL on failure.
 */
struct al_thread *al_os_thread_create(const char *name,
	void *stack, size_t stack_size, enum al_os_thread_pri pri,
	void (*thread_main)( void *arg), void *arg);

/**
 * Suspend a thread
 *
 * \param thread is a pointer to the thread.
 *
 * \returns 0 if the thread is suspended, others on failure.
 */
int al_os_thread_suspend(struct al_thread *thread);

/**
 * Resume a thread
 *
 * \param thread is a pointer to the thread.
 *
 * \returns 0 if the thread is resumed, others on failure.
 */
int al_os_thread_resume(struct al_thread *thread);

/**
 * Get the priority of a thread
 *
 * \param thread is a pointer to the thread.
 *
 * \returns the priority
 */
enum al_os_thread_pri al_os_thread_get_priority(struct al_thread *thread);

/**
 * Set the priority of a thread
 *
 * \param thread is a pointer to the thread.
 * \param pri is the priority.
 */
void al_os_thread_set_priority(struct al_thread *thread,
	enum al_os_thread_pri pri);

/**
 * Set the exit code for the specified thread
 *
 * \param thread is a pointer to the thread.
 * \param code is the exit code.
 */
void al_os_thread_set_exit_code(struct al_thread *thread, u32 code);

/**
 * Get exit flag of the specified thread
 * Generally, it is used in the loop of the thread itself to determine to
 * quit or not.
 *
 * \param thread is a pointer to the thread.
 *
 * \returns non zero means to exit the thread loop.
 */
int al_os_thread_get_exit_flag(struct al_thread *thread);

/**
 * Terminate the specified thread
 * Generally, it is called by another thread that want to terminated the
 * specified thread. Current thread can use this function to terminated
 * itself.
 *
 * \param thread is a pointer to the thread to be terminated.
 */
void al_os_thread_terminate(struct al_thread *thread);

/**
 * Terminate the specified thread synchronously
 * This function set exit-flag for the thread, wait the thread being
 * terminated, get the thread's exit-code, free memory of the thread,
 * at last return the exit-code. Do not use this function to terminate
 * the current thread itself.
 *
 * \param thread is a pointer to the thread to be terminated.
 *
 * \returns exit-code of the thread.
 */
u32 al_os_thread_terminate_with_status(struct al_thread *thread);

/**
 * Block the current thread to wait for a thread terminated
 *
 * \param thread is a pointer to the thread.
 *
 * \returns exit-code of the thread.
 */
u32 al_os_thread_join(struct al_thread *thread);

/**
 * Get the structure of the current thread.
 *
 * \returns pointer to the thread structure or NULL on failure.
 */
struct al_thread *al_os_thread_self(void);

/**
 * Suspends execution of the calling thread for (at least) specified
 * milliseconds.
 *
 * \param ms is suspends period in millisecond.
 */
void al_os_thread_sleep(int ms);

#endif /* __AYLA_AL_COMMON_OS_THREAD_H__ */
