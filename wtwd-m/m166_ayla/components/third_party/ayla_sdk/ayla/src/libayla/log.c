/*
 * Copyright 2011-2012 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <stdio.h>
#include <al/al_clock.h>
#include <ayla/clock.h>

#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/conf.h>
#include <ayla/log.h>
#include <ayla/timer.h>
#include <ayla/tty.h>
#include <ayla/snprintf.h>
#include <ayla/nameval.h>

#include <al/al_cli.h>
#include <al/al_log.h>
#ifdef ADA_BUILD_LOG_LOCK
#include <al/al_os_lock.h>
#endif
#ifdef ADA_BUILD_LOG_THREAD_CT
#include <al/al_os_thread.h>
#endif

/*
 * Optionally protect the log lines with locks if desired for debugging.
 * One thing to watch out for: fatal traps taken during logging could hang
 * trying to get this lock.  So, don't use for production yet.
 */
#ifdef ADA_BUILD_LOG_LOCK
struct al_lock *log_lock;	/* protects log_line and serializes log */
#endif /* ADA_BUILD_LOG_LOCK */

enum log_mask log_mask_minimum;
static char log_line[LOG_LINE];
const char *log_prefix = "";
static u8 log_enabled = 1;
u8 log_client_conf_enabled = 1;
struct log_mod log_mods[LOG_MOD_CT];

static int (*log_remote_print)(void *, const char *);
static void *log_remote_arg;

void print_remote_set(int (*print_func)(void *, const char *), void *arg)
{
	log_remote_print = print_func;
	log_remote_arg = arg;
}

const char log_sev_chars[] = LOG_SEV_CHARS;

static const char * const log_sevs[] = LOG_SEVS;

#ifdef ADA_BUILD_LOG_THREAD_CT
/* NULL-terminated */
static struct name_val log_tasks[ADA_BUILD_LOG_THREAD_CT + 1];

/*
 * Set thread ID string which will be put in log lines for the current thread.
 * Keep it short.  Name string is not copied.
 */
void log_thread_id_set(const char *name)
{
	struct name_val *nv;
	int task_id;

	task_id = (int)al_os_thread_self();
	if (task_id == 0) {
		return;
	}

#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_lock(log_lock);
#endif
	for (nv = log_tasks; nv < &log_tasks[ARRAY_LEN(log_tasks) - 1]; nv++) {
		if (!nv->name || nv->val == task_id) {
			nv->name = name;
			nv->val = task_id;
			break;
		}
	}
#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_unlock(log_lock);
#endif
	if (nv >= &log_tasks[ARRAY_LEN(log_tasks) - 1]) {
		log_err(LOG_MOD_DEFAULT, "log_tasks table is full");
	}
}

/*
 * Unset thread ID string
 */
void log_thread_id_unset(void)
{
	struct name_val *nv;
	int task_id;

	task_id = (int)al_os_thread_self();
	if (task_id == 0) {
		return;
	}

#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_lock(log_lock);
#endif
	for (nv = log_tasks; nv < &log_tasks[ARRAY_LEN(log_tasks) - 1]; nv++) {
		if (!nv->name || nv->val == task_id) {
			break;
		}
	}
	for (; nv < &log_tasks[ARRAY_LEN(log_tasks) - 1]; nv++) {
		*nv = *(nv + 1);
		if (!nv->name) {
			break;
		}
	}
#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_unlock(log_lock);
#endif
}

static const char *log_thread_id(void)
{
	return lookup_by_val(log_tasks, (int)al_os_thread_self());
}
#endif /* ADA_BUILD_LOG_THREAD_CT */

/*
 * Return the string for a log level.  Used by log_client.
 */
const char *log_sev_get(enum log_sev sev)
{
	if (sev >= LOG_SEV_LIMIT) {
		return ".";
	}
	return log_sevs[sev];
}

/*
 * Check if a log mod & sev is enabled
 */
int log_mod_sev_is_enabled(u8 mod_nr, enum log_sev sev)
{
	u32 mask;

	mod_nr &= LOG_MOD_MASK;
	mask = mod_nr < LOG_MOD_CT ? log_mods[mod_nr].mask : ~0;
	return mask & ((u32)1 << sev);
}

/*
 * Put log message into log_line buffer.
 */
size_t log_put_va_sev(u8 mod_nr, enum log_sev sev,
	const char *fmt, ADA_VA_LIST args)
{
	size_t rlen;
	size_t len;
	size_t rc;
	char time_stamp[CLOCK_FMT_LEN];
	u32 rough_time;
	static u32 last_rough_time;
	u64 mtime;
	u32 sec;
	u32 msec = 0;
	const char *mod_name;
	char *body;
	char *msg;
	enum al_clock_src clk_src;
#ifdef ADA_BUILD_LOG_THREAD_CT
	const char *thread_id;
#endif

	if (*fmt == LOG_EXPECTED[0]) {
		fmt++;
	} else if (!log_mod_sev_is_enabled(mod_nr, sev)) {
		return 0;
	}
	rlen = sizeof(log_line) - 1;
	len = 0;

	sec = al_clock_get(&clk_src);
	mtime = al_clock_get_total_ms();
	msec = mtime % 1000;

#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_lock(log_lock);
#endif
	if (mod_nr != LOG_MOD_NONE) {
		len = snprintf(log_line, rlen, "%s", log_prefix);
	}
	if (clk_src > AL_CS_DEF) {
		clock_fmt(time_stamp, sizeof(time_stamp), sec);

		/*
		 * Show full date once an hour or if full date
		 * hasn't been shown, otherwise just show hh:mm:ss + ms.
		 */
		rough_time = sec / (60 * 60);
		len += snprintf(log_line + len, rlen, "%s.%3.3lu ",
		    &time_stamp[rough_time == last_rough_time ?
		    CLOCK_FMT_TIME : 0],
		    msec);
		last_rough_time = rough_time;
	} else {
		len += snprintf(log_line + len, rlen, "%llu ", mtime);
	}
	if (sev != LOG_SEV_NONE) {
		len += snprintf(log_line + len, rlen - len,
#ifdef ADA_BUILD_LOG_SEV_SHORT
		    "%c ",
		    log_sev_chars[sev]);
#else
		    "%s:  ",
		    log_sev_get(sev));
#endif /* ADA_BUILD_LOG_SEV_SHORT */
	}

#ifdef LOG_THREAD /* add first letter of task name */
	{
		signed char task = pcTaskGetTaskName(NULL)[0];

		if (task != 't') {	/* omit tcpip_thread */
			len += snprintf(log_line + len, rlen - len, "(%c) ",
			    task);
		}
	}
#endif /* LOG_THREAD */

#ifdef ADA_BUILD_LOG_THREAD_CT
	thread_id = log_thread_id();
	if (thread_id) {
		len += snprintf(log_line + len, rlen - len, "%s ", thread_id);
	}
#endif

	mod_name = al_log_get_mod_name(mod_nr);
	if (mod_name) {
		len += snprintf(log_line + len, rlen - len, "%s: ", mod_name);
	}
	body = log_line + len;
	len += libayla_vsnprintf(body, rlen - len, fmt, args);

#ifndef ADA_BUILD_LOG_NO_BUF
	if (log_enabled && !(mod_nr & LOG_MOD_NOSEND)) {
		log_buf_put(mod_nr, sev, mtime, ct.ct_sec, msec, body,
		    len - (body - log_line));
	}
#endif

	if (sev != LOG_SEV_NONE) {
		if (rlen > len && log_line[len - 1] != '\n') {
			log_line[len++] = '\n';
		} else if (rlen == len) {
			strcpy(log_line + len - 5, " ...\n");
		}
	}
	log_line[len] = '\0';
	msg = log_line;
	rc = len;
#ifdef ADA_BUILD_LOG_SERIAL
	al_log_print(msg);
#endif
#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_unlock(log_lock);
#endif
	return rc;
}

/*
 * Put log message into log_line buffer.
 */
size_t log_put_va(u8 mod_nr, const char *fmt, ADA_VA_LIST args)
{
	u8 sev;

	sev = *(u8 *)fmt;		/* first char of fmt may be severity */
	if (sev >= LOG_BASE && sev < LOG_BASE + LOG_SEV_LIMIT) {
		fmt++;
		sev -= LOG_BASE;
	} else {
		sev = LOG_SEV_NONE;
	}
	return log_put_va_sev(mod_nr, sev, fmt, args);
}

/*
 * Put into log from LwIP thread.
 */
void log_put_raw(const char *fmt, ...)
{
	ADA_VA_LIST args;
	ADA_VA_START(args, fmt);
	log_put_va(LOG_MOD_DEFAULT, fmt, args);
	ADA_VA_END(args);
}

void log_put(const char *fmt, ...)
{
	ADA_VA_LIST args;
	ADA_VA_START(args, fmt);
	log_put_va(LOG_MOD_DEFAULT, fmt, args);
	ADA_VA_END(args);
}

void log_put_mod(u8 mod_nr, const char *fmt, ...)
{
	ADA_VA_LIST args;
	ADA_VA_START(args, fmt);
	log_put_va(mod_nr, fmt, args);
	ADA_VA_END(args);
}

void log_put_mod_sev(u8 mod_nr, enum log_sev sev, const char *fmt, ...)
{
	ADA_VA_LIST args;
	ADA_VA_START(args, fmt);
	log_put_va_sev(mod_nr, sev, fmt, args);
	ADA_VA_END(args);
}

void log_mask_init(enum log_mask mask)
{
	log_mask_change(NULL, mask, 0);
}

void log_mask_init_min(enum log_mask mask, enum log_mask min_mask)
{
	log_mask_minimum = min_mask;
	log_mask_init(mask);
}

void log_init(void)
{
#ifdef ADA_BUILD_LOG_LOCK
	log_lock = al_os_lock_create();
	ASSERT(log_lock);
#endif
	log_mask_init_min(BIT(LOG_SEV_INFO), LOG_DEFAULT);
	log_conf_init();
	conf_load(CONF_PNAME_LOG_MOD);
	al_cli_register("log", ada_log_cli_help, ada_log_cli);
}

#ifdef ADA_BUILD_FINAL
void log_final(void)
{
#ifdef ADA_BUILD_LOG_LOCK
	al_os_lock_destroy(log_lock);
#endif
}
#endif

/*
 * Enabling/disabling inserting log messages into buffer
 */
int log_enable(int enable)
{
	int old_val = log_enabled;

	log_enabled = enable;
	return old_val;
}

/*
 * Print out bytes as hex
 */
void log_bytes_in_hex(u8 mod_nr, void *buf, int len)
{
	char tmpbuf[48 + 12 + 1]; /* 16 * 3 + 12 */
	int i, j, off;

	for (i = 0; i < len; ) {
		off = 0;
		for (j = 0; j < 16 && i < len; j++) {
			off += snprintf(tmpbuf + off,
			    sizeof(tmpbuf) - off, "%2.2x ",
			    ((u8 *)buf)[i]);
			if ((j + 1) % 4 == 0) {
				off += snprintf(tmpbuf + off,
				    sizeof(tmpbuf) - off, " ");
			}
			i++;
		}
		log_put_mod_sev(mod_nr, LOG_SEV_DEBUG, "%s", tmpbuf);
	}
}

static int printcli_va(int add_nl, const char *fmt, ADA_VA_LIST args)
{
	char buf[512];
	size_t len;
	char *line;
	char *next;
	char saved = 0;

	len = vsnprintf(buf, sizeof(buf), fmt, args);

	if (len > sizeof(buf) - 2) {
		len = sizeof(buf) - 2;
	}
	if (add_nl && (len == 0 || buf[len - 1] != '\n')) {
		buf[len++] = '\n';
	}
	buf[len] = '\0';

	/*
	 * Split output at each newline.
	 */
	for (line = buf; line < &buf[len]; line = next) {
		next = strchr(line, '\n');
		if (next) {
			next++;
			saved = *next;
			*next = '\0';
		} else {
			next = NULL;
		}
		if (log_remote_print) {
			log_remote_print(log_remote_arg, line);
		} else {
			al_cli_print(line);
		}
		if (!next) {
			break;
		}
		*next = saved;
	}
	return len;
}

int printcli(const char *fmt, ...)
{
	ADA_VA_LIST  args;
	int len;

	ADA_VA_START(args, fmt);
	len = printcli_va(1, fmt, args);
	ADA_VA_END(args);
	return len;
}

/*
 * Note: printcli_s should be avoided.
 * It won't work on SDKs that don't provide for printing
 * less than one line at a time.
 */
int printcli_s(const char *fmt, ...)
{
	ADA_VA_LIST args;
	size_t len;

	ADA_VA_START(args, fmt);
	len = printcli_va(0, fmt, args);
	ADA_VA_END(args);
	return len;
}
