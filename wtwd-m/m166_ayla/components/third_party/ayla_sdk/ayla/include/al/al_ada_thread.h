/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_ADA_THREAD_H__
#define __AYLA_AL_ADA_THREAD_H__

#include <ayla/utypes.h>

/**
 * @file
 * ADA Thread Interfaces
 *
 * The ADA thread may be combined with another thread, such as the
 * LwIP TCP/IP thread, if desired.
 */

struct timer;

/**
 * Enter the ADA thread main loop.
 *
 * Call al_ada_kill() to quit the main loop.
 */
void al_ada_main_loop(void);

/**
 * Wakeup the main ADA thread.
 */
void al_ada_wakeup(void);

/**
 * Stop the main ADA thread.
 *
 * Indicate that the ADA thread should exit for cleanup.
 * This would probably be used only in test programs.
 * This does not wait for the ADA thread to exit before returning.
 */
void al_ada_kill(void);

/**
 * The callback structure.
 */
struct al_ada_callback {
	u8 pending;		/**< The pending flag */
	void (*func)(void *);	/**< The callback function */
	void *arg;		/**< The argument is passed to callback
				    function */
	struct al_ada_callback *next;
				/**< Link to next callback */
};

/**
 * Initialize the callback structure.
 *
 * \param cb is a callback structure.
 * \param func is a callback function.
 * \param arg is a argument to pass to the callback function.
 */
void al_ada_callback_init(struct al_ada_callback *cb,
		void (*func)(void *), void *arg);

/**
 * Call callback function.
 *
 * The function can be called on any thread, it puts the callback structure into
 * ada thread's callback queue, and then wakes up the ada thread, in ada thread
 * main loop, the callback function is called.
 *
 * \param cb is a callback structure.
 */
void al_ada_call(struct al_ada_callback *cb);

/**
 * Synchronized call callback function.
 *
 * If the function is called on ada thread, it is a nested call callback
 * function. If the function is called on other thread, it similars
 * al_ada_call, but the callback function with higher priority and the caller
 * thread is blocked until the callback returns.
 *
 * \param cb is a callback structure.
 */
void al_ada_sync_call(struct al_ada_callback *cb);

/**
 * Start the timer.
 *
 * \param timer points a structure that is already initialized by timer_init.
 * \param ms is delay in millisecond to trigger the callback.
 */
void al_ada_timer_set(struct timer *timer, unsigned long ms);

/**
 * Cancel the started timer.
 *
 * \param timer points structure that is already initialized by timer_init.
 */
void al_ada_timer_cancel(struct timer *timer);

#endif /* __AYLA_AL_ADA_THREAD_H__ */
