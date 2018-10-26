/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_ada_thread.h>
#include <ayla/assert.h>
#include "osal.h"
#include "lwip/sockets.h"
#include "ayla/timer.h"

#define igr_return(x) { __typeof__(x) __attribute__((unused)) d = (x); }

#define MAX_SELECT_SOCKETS 3

#define update_handler(handle, tmp) \
	if (tmp) { \
		handle = tmp; \
		tmp = NULL; \
	}

struct al_ada_sync_callback {
	struct al_ada_callback *cb;
	OsSemaphore sem;
	struct al_ada_sync_callback *next;
};

struct al_ada_thread {
	struct al_thread *ada_thread;
	int fd_wakeup[2];

	struct al_lock *fd_lock;
	struct {
		int fd;
		void (*ada_read)(int, void *);
		void (*ada_write)(int, void *);
		void (*exception)(int, void *);
		void *arg;
		int enable_read:1;
		int enable_write:1;

		void (*tmp_read)(int, void *);
		void (*tmp_write)(int, void *);
		void (*tmp_exception)(int, void *);
		void *tmp_arg;
	} fd_infos[MAX_SELECT_SOCKETS];
	int fd_max;

	struct al_lock *cb_queue_lock;
	struct al_ada_callback *cb_queue;
	struct al_ada_callback *cb_queue_tail;
	struct al_ada_sync_callback *sync_cb_queue;
	struct al_ada_sync_callback *sync_cb_queue_tail;

	struct timer_head timers;
};

static struct al_ada_thread al_ada_thread;
const char *al_pwb_linux_prompt_str = "PWB Linux> ";

static void wakeup_drop(int socket, void *arg)
{
	char dropbuf[32];

	igr_return(read(socket, dropbuf, sizeof(dropbuf)));
}

void al_ada_init(struct al_thread *thread)
{
	struct al_ada_thread *aat = &al_ada_thread;
	int i;

	memset(aat, 0, sizeof(*aat));
	aat->ada_thread = thread;
	for (i = 0; i < ARRAY_LEN(aat->fd_infos); i++) {
		aat->fd_infos[i].fd = -1;
	}

	aat->cb_queue_lock = al_os_lock_create();
	ASSERT(aat->cb_queue_lock);

	aat->fd_lock = al_os_lock_create();
	ASSERT(aat->fd_lock);

	al_ada_add_socket(aat->fd_wakeup[0], wakeup_drop, NULL,
	    NULL, NULL);

}

static int al_ada_search_fd_info(int socket, int find_empty)
{
	struct al_ada_thread *aat = &al_ada_thread;
	int empty_pos;
	int i;

	empty_pos = -1;
	for (i = 0; i < ARRAY_LEN(aat->fd_infos); i++) {
		if (aat->fd_infos[i].fd == socket) {
			return i;
		}
		if (empty_pos < 0 && aat->fd_infos[i].fd < 0) {
			empty_pos = i;
		}
	}

	if (find_empty && empty_pos >= 0) {
		return empty_pos;
	}

	return -1;
}

int al_ada_enable_write(int socket, int enable)
{
	struct al_ada_thread *aat = &al_ada_thread;
	int i;

	// al_os_lock_lock(aat->fd_lock);
	i = al_ada_search_fd_info(socket, 0);
	if (i >= 0) {
		aat->fd_infos[i].enable_write = enable ? 1 : 0;
		// al_ada_wakeup();
	}
	// al_os_lock_unlock(aat->fd_lock);

	return i < 0;
}

int al_ada_enable_read(int socket, int enable)
{
	struct al_ada_thread *aat = &al_ada_thread;
	int i;

	al_os_lock_lock(aat->fd_lock);
	i = pfm_pwb_linux_search_fd_info(socket, 0);
	if (i >= 0) {
		aat->fd_infos[i].enable_read = enable ? 1 : 0;
		// al_ada_wakeup();
	}
	al_os_lock_unlock(aat->fd_lock);

	return i < 0;
}

int al_ada_remove_socket(int socket)
{
	struct al_ada_thread *aat = &al_ada_thread;
	int i;
	// printf("al_os_lock_lock aat->fd_lock\n");
	// al_os_lock_lock(aat->fd_lock);
	i = al_ada_search_fd_info(socket, 0);
	if (i >= 0) {
		aat->fd_infos[i].fd = -1;
		aat->fd_infos[i].fd = -1;
		aat->fd_max = 0;
		// al_ada_wakeup();
	}
	// al_os_lock_unlock(aat->fd_lock);

	return i < 0;
}


int al_ada_add_socket(int socket,
		void (*ada_read)(int socket, void *arg),
		void (*ada_write)(int socket, void *arg),
		void (*exception)(int socket, void *arg),
		void *arg)
{
	struct al_ada_thread *aat = &al_ada_thread;
	int i;

	al_os_lock_lock(aat->fd_lock);
	i = al_ada_search_fd_info(socket, 1);
	if (i >= 0) {
		if (aat->fd_infos[i].fd >= 0) {
			aat->fd_infos[i].tmp_read = ada_read;
			aat->fd_infos[i].tmp_write = ada_write;
			aat->fd_infos[i].tmp_exception = exception;
			aat->fd_infos[i].tmp_arg = arg;
		} else {
			memset(&aat->fd_infos[i], 0, sizeof(aat->fd_infos[i]));
			aat->fd_infos[i].fd = socket;
			aat->fd_infos[i].ada_read = ada_read;
			aat->fd_infos[i].ada_write = ada_write;
			aat->fd_infos[i].exception = exception;
			aat->fd_infos[i].arg = arg;
			aat->fd_infos[i].enable_read = 1;
			aat->fd_max = 0;
		}
		// al_ada_wakeup();
	}
	al_os_lock_unlock(aat->fd_lock);

	return i < 0;
}

void al_ada_main_loop(void)
{
	struct al_ada_thread *aat = &al_ada_thread;
	u32 max_wait;
	int i;
	int err_times;
	fd_set read_fds;
	fd_set write_fds;
	fd_set exception_fds;
	struct timeval tv;
	struct al_ada_sync_callback *sync_cb;
	struct al_ada_callback *cb;
	err_times = 0;
	while(aat->ada_thread == NULL){
		printf("wait al ada init\n");
		OS_MsDelay(100);
	}

	while (!al_os_thread_get_exit_flag(aat->ada_thread)) {
		max_wait = timer_advance(&aat->timers);
		

		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		FD_ZERO(&exception_fds);
		al_os_lock_lock(aat->fd_lock);
		for (i = 0; i < ARRAY_LEN(aat->fd_infos); i++) {
			if (aat->fd_infos[i].fd >= 0) {
				update_handler(aat->fd_infos[i].ada_read,
				    aat->fd_infos[i].tmp_read);
				update_handler(aat->fd_infos[i].ada_write,
				    aat->fd_infos[i].tmp_write);
				update_handler(aat->fd_infos[i].exception,
				    aat->fd_infos[i].tmp_exception);
				update_handler(aat->fd_infos[i].arg,
				    aat->fd_infos[i].tmp_arg);

				if (aat->fd_infos[i].ada_read &&
				    aat->fd_infos[i].enable_read) {
					FD_SET(aat->fd_infos[i].fd,
					    &read_fds);
				}
				if (aat->fd_infos[i].ada_write &&
				    aat->fd_infos[i].enable_write) {
					FD_SET(aat->fd_infos[i].fd,
					    &write_fds);
				}
				if (aat->fd_infos[i].exception) {
					FD_SET(aat->fd_infos[i].fd,
					    &exception_fds);
				}
			}
		}
		if (aat->fd_max == 0) {
			for (i = 0; i < ARRAY_LEN(aat->fd_infos); i++) {
				if (aat->fd_infos[i].fd > aat->fd_max) {
					aat->fd_max = aat->fd_infos[i].fd;
				}
			}
			aat->fd_max ++;
		}
		al_os_lock_unlock(aat->fd_lock);

		// tv.tv_sec = max_wait / 1000;
		// tv.tv_usec = (max_wait % 1000) * 1000;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		// printf("ada thread start select\n");
		i = select(aat->fd_max, &read_fds, &write_fds,
		    &exception_fds, &tv);

		if (i < 0) {
			/* Closing socket in other thread causes the select()
			 * error, but the socket is removed from the select
			 * set, so we allow several times error
			 */
			if (err_times) {
				al_os_thread_sleep(100);
			}
			err_times++;
			ASSERT(err_times < 5);
			if(err_times >= 5){
				printf("error time 5 !\n");
			}
			continue;
		}
		err_times = 0;
		if (i == 0) {
			// continue;
		}

		al_os_lock_lock(aat->fd_lock);
		for (i = 0; i < ARRAY_LEN(aat->fd_infos); i++) {
			if (aat->fd_infos[i].fd >= 0) {
				if (FD_ISSET(aat->fd_infos[i].fd,&read_fds)) {
					aat->fd_infos[i].ada_read(
					    aat->fd_infos[i].fd,
					    aat->fd_infos[i].arg);
					if(aat->fd_infos[i].fd < 0)
						break;
				}
				if (FD_ISSET(aat->fd_infos[i].fd,
				    &write_fds)) {
					aat->fd_infos[i].ada_write(
					    aat->fd_infos[i].fd,
					    aat->fd_infos[i].arg);
					if(aat->fd_infos[i].fd < 0)
						break;
				}
				if (FD_ISSET(aat->fd_infos[i].fd,
				    &exception_fds)) {
					aat->fd_infos[i].exception(
					    aat->fd_infos[i].fd,
					    aat->fd_infos[i].arg);
					if(aat->fd_infos[i].fd < 0)
						break;
				}
			}
		}
		al_os_lock_unlock(aat->fd_lock);

		/* Process the sync callback */
		while (aat->sync_cb_queue) {
			al_os_lock_lock(aat->cb_queue_lock);
			sync_cb = aat->sync_cb_queue;
			aat->sync_cb_queue = sync_cb->next;
			if (aat->sync_cb_queue == NULL) {
				aat->sync_cb_queue_tail = NULL;
			}
			al_os_lock_unlock(aat->cb_queue_lock);

			sync_cb->cb->func(sync_cb->cb->arg);
			OS_SemSignal(sync_cb->sem);
		}

		/* Process the callback */
		while (aat->cb_queue) {
			al_os_lock_lock(aat->cb_queue_lock);
			cb = aat->cb_queue;
			aat->cb_queue = cb->next;
			if (aat->cb_queue == NULL) {
				aat->cb_queue_tail = NULL;
			}
			al_os_lock_unlock(aat->cb_queue_lock);
			ASSERT(cb->pending);
			cb->pending = 0;
			cb->func(cb->arg);
		}

		
	}
}

void al_ada_wakeup(void)
{
}

void al_ada_kill(void)
{
	struct al_ada_thread *aat = &al_ada_thread;

	al_os_thread_terminate(aat->ada_thread);
}

void al_ada_callback_init(struct al_ada_callback *cb,
		void (*func)(void *), void *arg)
{
	memset(cb, 0, sizeof(*cb));
	cb->func = func;
	cb->arg = arg;
}

void al_ada_call(struct al_ada_callback *cb)
{

	struct al_ada_thread *aat;

	ASSERT(cb);
	if (cb->pending) {
		return;
	}
	cb->pending = 1;

	aat = &al_ada_thread;
	al_os_lock_lock(aat->cb_queue_lock);
	if (aat->cb_queue_tail) {
		aat->cb_queue_tail->next = cb;
		aat->cb_queue_tail = cb;
	} else {
		aat->cb_queue = cb;
		aat->cb_queue_tail = cb;
	}
	cb->next = NULL;
	al_os_lock_unlock(aat->cb_queue_lock);

	al_ada_wakeup();
}

void al_ada_sync_call(struct al_ada_callback *cb)
{
	struct al_ada_thread *aat = &al_ada_thread;
	struct al_ada_sync_callback sync_cb;
	int rc;

	ASSERT(cb);

	if (aat->ada_thread == NULL) {
		AYLA_ASSERT_NOTREACHED();
	}

	if (aat->ada_thread == al_os_thread_self()) {
		cb->func(cb->arg);
		return;
	}

	memset(&sync_cb, 0, sizeof(sync_cb));
	sync_cb.cb = cb;
	rc = OS_SemInit(&sync_cb.sem, 1, 0);
	ASSERT(rc == 0);
	al_os_lock_lock(aat->cb_queue_lock);
	if (aat->sync_cb_queue_tail) {
		aat->sync_cb_queue_tail->next = &sync_cb;
		aat->sync_cb_queue_tail = &sync_cb;
	} else {
		aat->sync_cb_queue = &sync_cb;
		aat->sync_cb_queue_tail = &sync_cb;
	}
	cb->next = NULL;
	al_os_lock_unlock(aat->cb_queue_lock);
	al_ada_wakeup();
	OS_SemWait(sync_cb.sem,portMAX_DELAY);
	OS_SemDelete(sync_cb.sem);
}

void al_ada_timer_set(struct timer *timer, unsigned long ms)
{
	struct al_ada_thread *aat = &al_ada_thread;

	ASSERT(timer);
	ASSERT(0 == (ms & 0x80000000UL));

	timer_set(&aat->timers, timer, ms);
}

void al_ada_timer_cancel(struct timer *timer)
{
	struct al_ada_thread *aat = &al_ada_thread;

	ASSERT(timer);

	timer_cancel(&aat->timers, timer);
}
