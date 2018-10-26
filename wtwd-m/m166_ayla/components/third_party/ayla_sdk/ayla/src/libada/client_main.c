/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <ayla/conf.h>
#include <ayla/clock.h>
#include <al/al_ada_thread.h>
#include <al/al_clock.h>
#include <al/al_os_mem.h>
#include <ada/err.h>
#include <ada/client.h>
#include <ada/sched.h>
#include <ada/ada.h>

void client_lock_int(const char *func, int line)
{
}

void client_unlock_int(const char *func, int line)
{
}

struct client_callback_queue {
	struct al_ada_callback *head;
	struct al_ada_callback *tail;
};

struct client_callback_queue *client_callback_queue_new(unsigned int len)
{
	struct client_callback_queue *queue;

	queue = al_os_mem_calloc(sizeof(struct client_callback_queue));
	return queue;
}

#ifdef ADA_BUILD_FINAL
void client_callback_queue_free(struct client_callback_queue *queue)
{
	al_os_mem_free(queue);
}
#endif

int client_callback_enqueue(struct client_callback_queue *queue,
		struct al_ada_callback *cb)
{
	ASSERT(queue);

	if (cb->pending) {
		return 0;
	}
	cb->pending = 1;

	if (queue->tail) {
		queue->tail->next = cb;
		queue->tail = cb;
	} else {
		queue->head = cb;
		queue->tail = cb;
	}
	cb->next = NULL;

	return 0;
}

struct al_ada_callback *client_callback_dequeue(
		struct client_callback_queue *queue)
{
	struct al_ada_callback *cb;

	ASSERT(queue);

	cb = queue->head;
	if (queue->head) {
		queue->head = queue->head->next;
		if (queue->head == NULL) {
			queue->tail = NULL;
		}
	}
	return cb;
}

int ada_init(const struct ada_conf *conf)
{
	struct ada_conf *cf = &ada_conf;

	/* Setup conf environment */
	*cf = *conf;
	cf->enable = 1;
	client_conf_init();
	ada_conf_init();
	oem_conf_init();

	/* Init the default clock */
	al_clock_set(CLOCK_START, AL_CS_DEF);
	client_init();

	conf_load(NULL);
	return 0;
}

#ifdef ADA_BUILD_FINAL
void ada_final(void)
{
	ada_client_down();
}
#endif
